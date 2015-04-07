#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/pagemap.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/log2.h>
#include <linux/slab.h>
#include <linux/dad.h>
#include <linux/fs.h>
#include <linux/mm.h>

#include <asm/uaccess.h>

#include "flash_driver.h"

/* device struct */
struct flash_device {
	struct cdev cdev;           /* char devices structure */
	struct dad_device *dad_dev; /* parent device */
	struct device *dev;         /* associated device */
	struct module *module;
	struct mutex lock;
	struct completion completion;
	void __iomem *iomem;        /* mmapped registers */
	size_t max_size;            /* Maximum buffer size to be DMA'd to/from in bytes */
	int number;
	//int irq;
};

struct flash_file {
	struct flash_device *dev;
	void *vbuf; /* virtual address of physically contiguous buffer */
	dma_addr_t dma_handle; /* physical address of the same buffer */
	size_t size;
};

static const struct dad_device_id flash_ids[] = {
	{ FLASH_SYNC_DEV_ID },
	{ },
};

static struct class *flash_class;
static dev_t flash_devno;
static dev_t flash_n_devices;

static irqreturn_t flash_irq(int irq, void *dev)
{
	struct flash_device *flash = dev;
	u32 cmd_reg;

	// printf("IRQ called\n");
	cmd_reg = ioread32(flash->iomem + FLASH_REG_CMD);
	cmd_reg >>= FLASH_CMD_IRQ_SHIFT;
	cmd_reg &= FLASH_CMD_IRQ_MASK;

	if (cmd_reg == FLASH_CMD_IRQ_DONE) {
		complete_all(&flash->completion);
		iowrite32(0, flash->iomem + FLASH_REG_CMD);
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}

static bool flash_access_ok(struct flash_device *flash,
			const struct flash_access *access)
{
	unsigned max_sz = ioread32(flash->iomem + FLASH_REG_MAX_SIZE);
	unsigned size = access->size;
	// printf("Max size: %d, current size: %d\n", max_sz, size);
	if (size > max_sz || size <= 0)
		return false;

	return true;
}

static int flash_transfer(struct flash_device *flash,
			struct flash_file *file,
			const struct flash_access *access)
{
	/* compute the input and output burst */
	int wait;

	// unsigned sz = access->size;
	// unsigned num = access->num_samples;

	size_t in_buf_size = FLASH_INPUT_SIZE;
	size_t out_buf_size = FLASH_OUTPUT_SIZE; 

	// printf("in_buf_size: %d\n", in_buf_size); 
	// printf("out_buf_size: %d\n", out_buf_size); 

	INIT_COMPLETION(flash->completion);

	if (!flash_access_ok(flash, access))
		return -EINVAL;

	iowrite32(file->dma_handle, flash->iomem + FLASH_REG_SRC);
	iowrite32(file->dma_handle + in_buf_size, flash->iomem + FLASH_REG_DST);
	iowrite32(FLASH_SIZE, flash->iomem + FLASH_REG_SIZE);
	iowrite32(0x1, flash->iomem + FLASH_REG_CMD);

	wait = wait_for_completion_interruptible(&flash->completion);
	if (wait < 0)
		return -EINTR;
	return 0;
}

static int flash_access_ioctl(struct flash_device *flash,
			struct flash_file *file,
			void __user *arg)
{
	struct flash_access access;

	// printf("Copying from user\n");
	if (copy_from_user(&access, arg, sizeof(access)))
		return -EFAULT;
	// printf("Copying successful\n");

	// printf("Checking access\n");
	if (!flash_access_ok(flash, &access))
		return -EINVAL;
	// printf("Access is ok\n");

	// printf("Locking mutex\n");
	if (mutex_lock_interruptible(&flash->lock))
		return -EINTR;
	// printf("Lock acquired\n");

	flash_transfer(flash, file, &access);
	mutex_unlock(&flash->lock);

	return 0;
}

static long flash_do_ioctl(struct file *file, unsigned int cm, void __user *arg)
{
	struct flash_file *priv = file->private_data;
	struct flash_device *flash = priv->dev;

	switch (cm) {
	case FLASH_IOC_ACCESS:
		// printf("FLASH_IOC_ACCESS\n");
		return flash_access_ioctl(flash, priv, arg);
	default:
		return -ENOTTY;
	}
}

static long flash_ioctl(struct file *file, unsigned int cm, unsigned long arg)
{
	return flash_do_ioctl(file, cm, (void __user *)arg);
}

static int flash_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct flash_file *priv = file->private_data;
	struct flash_device *flash = priv->dev;
	unsigned long pfn;
	size_t size;

	size = vma->vm_end - vma->vm_start;
	if (size > priv->size) {
		dev_info(flash->dev, "size: %zu, max size: %zu\n", size, priv->size);
		return -EINVAL;
	}
	pfn = page_to_pfn(virt_to_page(priv->vbuf));
	return remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot);
}

static int flash_open(struct inode *inode, struct file *file)
{
	struct flash_file *priv;
	struct flash_device *flash;
	int rc;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;

	flash = container_of(inode->i_cdev, struct flash_device, cdev);
	priv->dev = flash;
	priv->size = flash->max_size;

	priv->vbuf = dma_alloc_coherent(NULL, priv->size, &priv->dma_handle, GFP_KERNEL);
	if (priv->vbuf == NULL) {
		dev_err(flash->dev, "cannot allocate contiguous DMA buffer of size %zu\n", priv->size);
		rc = -ENOMEM;
		goto err_dma_alloc;
	}

	if (!try_module_get(flash->module)) {
		rc = -ENODEV;
		goto err_module_get;
	}

	file->private_data = priv;
	return 0;

err_module_get:
	dma_free_coherent(NULL, priv->size, priv->vbuf, priv->dma_handle);
err_dma_alloc:
	kfree(priv);
	return rc;
}

static int flash_release(struct inode *inode, struct file *file)
{
	struct flash_file *priv = file->private_data;
	struct flash_device *flash = priv->dev;

	module_put(flash->module);
	dma_free_coherent(NULL, priv->size, priv->vbuf, priv->dma_handle);
	kfree(priv);
	return 0;
}

/*
 * pointers to functions defined by the driver that perform various operation
 * over the device
 */
static const struct file_operations flash_fops = {
	.owner           = THIS_MODULE,
	.open            = flash_open,
	.release         = flash_release,
	.unlocked_ioctl  = flash_ioctl,
	.mmap            = flash_mmap,
};

static int flash_create_cdev(struct flash_device *flash, int ndev)
{
	dev_t devno = MKDEV(MAJOR(flash_devno), ndev);
	int rc;

	/* char device registration */
	cdev_init(&flash->cdev, &flash_fops);
	flash->cdev.owner = THIS_MODULE;
	rc = cdev_add(&flash->cdev, devno, 1);
	if (rc) {
		dev_err(flash->dev, "Error %d adding cdev %d\n", rc, ndev);
		goto out;
	}

	flash->dev = device_create(flash_class, flash->dev, devno, NULL, "flash.%i", ndev);
	if (IS_ERR(flash->dev)) {
		rc = PTR_ERR(flash->dev);
		dev_err(flash->dev, "Error %d creating device %d\n", rc, ndev);
		flash->dev = NULL;
		goto device_create_failed;
	}

	dev_set_drvdata(flash->dev, flash);
	return 0;

device_create_failed:
	cdev_del(&flash->cdev);
out:
	return rc;
}

static void flash_destroy_cdev(struct flash_device *flash, int ndev)
{
	dev_t devno = MKDEV(MAJOR(flash_devno), ndev);

	device_destroy(flash_class, devno);
	cdev_del(&flash->cdev);
}

static int flash_probe(struct dad_device *dev)
{
	struct flash_device *flash;
	int dev_id;
	int rc;
	unsigned max_sz;

	flash = kzalloc(sizeof(*flash), GFP_KERNEL);
	if (flash == NULL)
		return -ENOMEM;
	flash->module = THIS_MODULE;
	flash->dad_dev = dev;
	flash->number = flash_n_devices;
	mutex_init(&flash->lock);
	init_completion(&flash->completion);

	flash->iomem = ioremap(dev->addr, dev->length);
	if (flash->iomem == NULL) {
		rc = -ENOMEM;
		goto err_ioremap;
	}

	dev_id = ioread32(flash->iomem + FLASH_REG_ID);
	if (dev_id != FLASH_SYNC_DEV_ID) {
		rc = -ENODEV;
		goto err_reg_read;
	}

	rc = flash_create_cdev(flash, flash->number);
	if (rc)
		goto err_cdev;

	rc = request_irq(dev->irq, flash_irq, IRQF_SHARED, "flash", flash);
	if (rc) {
		dev_info(flash->dev, "cannot request IRQ number %d\n", -EMSGSIZE);
		goto err_irq;
	}

	flash_n_devices++;
	dev_set_drvdata(&dev->device, flash);

	max_sz = ioread32(flash->iomem + FLASH_REG_MAX_SIZE);
	flash->max_size = round_up(FLASH_BUF_SIZE, PAGE_SIZE);

	dev_info(flash->dev, "device registered.\n");

	return 0;

err_irq:
	flash_destroy_cdev(flash, flash->number);
err_cdev:
	iounmap(flash->iomem);
err_reg_read:
	iounmap(flash->iomem);
err_ioremap:
	kfree(flash);
	return rc;
}

static void __exit flash_remove(struct dad_device *dev)
{
	struct flash_device *flash = dev_get_drvdata(&dev->device);

	/* free_irq(dev->irq, flash->dev); */
	flash_destroy_cdev(flash, flash->number);
	iounmap(flash->iomem);
	kfree(flash);
	dev_info(flash->dev, "device unregistered.\n");
}

static struct dad_driver flash_driver = {
	.probe    = flash_probe,
	.remove    = flash_remove,
	.name = DRV_NAME,
	.id_table = flash_ids,
};

static int __init flash_sysfs_device_create(void)
{
	int rc;

	flash_class = class_create(THIS_MODULE, "flash");
	if (IS_ERR(flash_class)) {
		printk(KERN_ERR PFX "Failed to create flash class\n");
		rc = PTR_ERR(flash_class);
		goto out;
	}

	/*
	 * Dynamically allocating device numbers.
	 *
	 * The major and minor numbers are global variables
	 */
	rc = alloc_chrdev_region(&flash_devno, 0, FLASH_MAX_DEVICES, "flash");
	if (rc) {
		printk(KERN_ERR PFX "Failed to allocate chrdev region\n");
		goto alloc_chrdev_region_failed;
	}

	return 0;

alloc_chrdev_region_failed:
	class_destroy(flash_class);
out:
	return rc;
}

static void flash_sysfs_device_remove(void)
{
	dev_t devno = MKDEV(MAJOR(flash_devno), 0);

	class_destroy(flash_class);
	unregister_chrdev_region(devno, FLASH_MAX_DEVICES); /* freeing device numbers */
}

/* initialization function */
static int __init flash_init(void)
{
	int rc;

	printk(KERN_INFO "Device-driver initialization\n");
	rc = flash_sysfs_device_create();
	if (rc)
		return rc;

	rc = dad_register_driver(&flash_driver);
	if (rc)
		goto err;

	return 0;

err:
	flash_sysfs_device_remove();
	return rc;
}

/* shutdown function */
static void __exit flash_exit(void)
{
	printk(KERN_INFO "Device-driver shutdown\n");
	dad_unregister_driver(&flash_driver);
	flash_sysfs_device_remove();
}

module_init(flash_init) /* register initialization function */
module_exit(flash_exit) /* register shutdown function */

MODULE_AUTHOR("Paolo Mantovani <paolo@cs.columbia.edu>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("flash driver");


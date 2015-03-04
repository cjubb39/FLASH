#include "device-list.h"
#include "alloc.h"

#include "flash-sync.h"

static void flash_release(struct object *obj)
{
	struct flash_sync *flash = obj_to_flash(obj);

	free(flash);
}

static int flash_create(const struct device_desc *desc, const char *name)
{
	struct flash_sync *flash;

	flash = cargo_zalloc(sizeof(*flash));
	if (flash == NULL)
		return -1;

	flash->dev.obj.release = flash_release;
	flash->dev.obj.name = name;
	flash->dev.id = desc->id;
	flash->dev.length = FLASH_NR_REGS * sizeof(u32);

	if (device_sync_register(&flash->dev, flash_main)) {
		free(flash);
		return -1;
	}

	return 0;
}

const struct device_init_operations flash_sync_init_ops = {
	.create		= flash_create,
};


#ifndef _FLASH_H_
#define _FLASH_H_

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
#ifndef __user
#define __user
#endif
#endif /* __KERNEL__ */

#include "flash_sched.h"

#define DRV_NAME  "flash"
#define PFX    DRV_NAME ": "
#define FLASH_MAX_DEVICES  64

#define FLASH_REG_CMD        0x00
#define FLASH_REG_SRC        0x04
#define FLASH_REG_DST        0x08
#define FLASH_REG_MAX_SIZE   0x0c
#define FLASH_REG_ID         0x10

#define FLASH_CMD_IRQ_SHIFT  4
#define FLASH_CMD_IRQ_MASK   0x3
#define FLASH_CMD_IRQ_DONE   0x2

#define FLASH_CMD_RESET		 0x1

#define FLASH_SYNC_DEV_ID    0x2

/* Determine buffer size (bytes) */
#define FLASH_INPUT_NUM 4
#define FLASH_OUTPUT_NUM 4
#define FLASH_INPUT_SIZE	(sizeof(flash_task_t) * FLASH_INPUT_NUM)
#define FLASH_OUTPUT_SIZE	(sizeof(flash_task_t) * FLASH_OUTPUT_NUM)

#define FLASH_BUF_SIZE	(FLASH_INPUT_SIZE + FLASH_OUTPUT_SIZE)

#define FLASH_IOC_ACCESS     _IOW ('I', 0, struct flash_access)
#define FLASH_IOC_READ       _IOR ('I', 0, struct flash_access)

#endif /* _FLASH_H_ */

#ifndef FLASH_SYNC_H
#define FLASH_SYNC_H

#include "device.h"

// Registers: must match the list of registers in the driver
enum flash_regs {
  FLASH_REG_CMD,         // Command and status register
                       // Write 0x1 to start computation
                       // Write 0x0 to reset the device
  FLASH_REG_SRC,
  FLASH_REG_DST,
  FLASH_REG_MAX_SIZE,    // READ-ONLY maximum allowed size
  FLASH_REG_ID,          // device ID assigned by OS.
  FLASH_NR_REGS,
};

// Status bits in FLASH_REG_CMD
#define STATUS_RUN  BIT(4); // flash running
                            // error if both set
#define FLASH_CMD_RESET 0x1

struct flash_sync {
  struct device dev;
};

void flash_main(struct device *dev);

static inline struct flash_sync *dev_to_flash(struct device *device)
{
  return container_of(device, struct flash_sync, dev);
}

static inline struct flash_sync *obj_to_flash(struct object *object)
{
  struct device *dev = obj_to_device(object);

  return dev_to_flash(dev);
}

#endif /* FLASH_SYNC_H */

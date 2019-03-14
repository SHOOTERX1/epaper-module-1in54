/**
 * SHOOTERX1 <yorha.a2@foxmail.com>
 */
#if !defined(EPAPER_CMDS_H)
#define EPAPER_CMDS_H

#include <linux/ioctl.h>

#define EPAPER_MAGIC 0x25

#define EPAPER_IS_DEV_BUSY              _IO(EPAPER_MAGIC, 0)
#define EPAPER_DC_PIN_SET_HIGH          _IO(EPAPER_MAGIC, 1)
#define EPAPER_DC_PIN_SET_LOW           _IO(EPAPER_MAGIC, 2)
#define EPAPER_RESET                    _IO(EPAPER_MAGIC, 3)

#endif // EPAPER_CMDS_H
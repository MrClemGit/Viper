#ifndef __PORT_FLASH__
#define __PORT_FLASH__

#include "flash_efc.h"

#define FLASH_ERR_OK            0
#define FLASH_ERR_BAD_ADDRESS   1
#define FLASH_ERR_UNALIGNED     2
#define FLASH_ERR_CANT_UNLOCK   3
#define FLASH_ERR_CANT_LOCK     4
#define FLASH_ERR_WRITE_FAIL    5


uint32_t flash_write_data(uint32_t *flashStart, uint32_t *data, uint32_t dataLength);

uint32_t* flash_write_buffer(uint32_t *flashStart, uint8_t *data, uint32_t dataLength);


#endif
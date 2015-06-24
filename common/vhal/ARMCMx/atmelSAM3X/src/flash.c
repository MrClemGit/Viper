
#include "flash.h"


#define FLASH_WRITE_STEP  64

uint32_t* flash_write_buffer(uint32_t *flashStart, uint8_t *data, uint32_t dataLength) {
    uint32_t wrt = 0;
    uint32_t *curpos = flashStart;

    while (wrt < dataLength) {
        if (wrt + FLASH_WRITE_STEP <= dataLength) {
            if (flash_write_data(curpos, (uint32_t*)(data + wrt), FLASH_WRITE_STEP / sizeof(uint32_t)))
                return (uint32_t*)FLASH_ERR_WRITE_FAIL;
            wrt += FLASH_WRITE_STEP;
            curpos+= FLASH_WRITE_STEP / sizeof(uint32_t);
        } else {
            int rem = (dataLength - wrt);
            rem = (rem % sizeof(uint32_t)) ? (rem + (sizeof(uint32_t) - (rem % sizeof(uint32_t)))) : (rem);
            if (flash_write_data(curpos, data + wrt, rem / sizeof(uint32_t)))
                return (uint32_t*)FLASH_ERR_WRITE_FAIL;
            wrt += rem;
            curpos+=(rem/sizeof(uint32_t));
        }        
    }
    return curpos;
}

uint32_t flash_write_data(uint32_t *flashStart, uint32_t *data, uint32_t dataLength) {
    uint32_t retCode;
    uint32_t byteLength = dataLength * sizeof(uint32_t);

    if ((uint32_t)flashStart < IFLASH1_ADDR) {
        return FLASH_ERR_BAD_ADDRESS;
    }

    if ((uint32_t)flashStart >= (IFLASH1_ADDR + IFLASH1_SIZE)) {
        return FLASH_ERR_BAD_ADDRESS;
    }

    if (((uint32_t)flashStart & 3) != 0) {
        return FLASH_ERR_UNALIGNED;
    }

    // Unlock page

    retCode = flash_unlock((uint32_t)flashStart, (uint32_t)flashStart + byteLength - 1, 0, 0);
    if (retCode != FLASH_RC_OK) {
        return FLASH_ERR_CANT_UNLOCK;
    }

    // write data

    retCode = flash_write((uint32_t)flashStart, data, byteLength, 1);

    if (retCode != FLASH_RC_OK) {
        return FLASH_ERR_WRITE_FAIL;
    }

    // Lock page

    retCode = flash_lock((uint32_t)flashStart, (uint32_t)flashStart + byteLength - 1, 0, 0);
    if (retCode != FLASH_RC_OK) {
        return FLASH_ERR_CANT_LOCK;
    }
    return FLASH_ERR_OK;
}
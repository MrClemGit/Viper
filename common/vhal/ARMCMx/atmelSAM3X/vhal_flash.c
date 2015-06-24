#include "vhal.h"
#include "flash.h"


int vhalFlashGetSector(void *addr) {
	(void)addr;
	return 0;
}

int vhalFlashErase(void *addr, uint32_t size) {
	(void)addr;
	(void)size;
	return 0;
}

int vhalFlashWrite(void *addr, uint8_t *data, uint32_t len) {
	return (addr > (uint8_t*)flash_write_buffer(addr, data, len)) ? 0 : 1;
}

void *vhalFlashAlignToSector(void *addr) {
	uint32_t a = (uint32_t)addr;
	if (a % IFLASH_PAGE_SIZE) {
		a += IFLASH_PAGE_SIZE - (a % IFLASH_PAGE_SIZE);
	}
	return (void *)a;
}

#include "vhal.h"



int vhalFlashGetSector(void *addr) {
	(void)addr;
    return -1;
}

int vhalFlashErase(void *addr, uint32_t size) {
	(void)addr;
	(void)size;
    return 0;
}

int vhalFlashWrite(void *addr, uint8_t *data, uint32_t len) {
	(void)addr;
	(void)data;
	(void)len;
    return 0;
}

void *vhalFlashAlignToSector(void *addr) {
	(void)addr;
    return NULL;
}

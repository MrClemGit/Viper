#include "vhal.h"
#include "vhal_gpio.h"



int vhalInitSPI(void *data) {
	(void)data;
	return 0;
}

int vhalSpiInit(uint32_t spi, vhalSpiConf *conf) {
	return 0;
}

int vhalSpiLock(uint32_t spi) {
	return 0;
}
int vhalSpiUnlock(uint32_t spi) {
	return 0;
}

int vhalSpiSelect(uint32_t spi) {
	return 0;
}
int vhalSpiUnselect(uint32_t spi) {
	return 0;
}


int vhalSpiExchange(uint32_t spi, void *tosend, void *toread, uint32_t blocks) {
	return 0;
}

int vhalSpiDone(uint32_t spi) {
	return 0;
}

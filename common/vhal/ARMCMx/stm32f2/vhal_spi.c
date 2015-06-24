#include "vhal.h"
#include "vhal_gpio.h"
#include "hal.h"


//smt32f4 has max 6 spi peripherals

static vhalSpiConf *spi_conf[6];
static VSemaphore spi_sem[6];
static uint8_t spi_status[6];
static SPIDriver *spidrv[6];
static SPIConfig spicfg[6];


/*
typedef struct _vhal_spi_conf {
  uint32_t clock;
  uint16_t miso;
  uint16_t mosi;
  uint16_t sclk;
  uint16_t nss;
  uint8_t mode;
  uint8_t bits;
  uint8_t master;
  uint8_t unused;
} vhalSpiConf;
*/



int vhalInitSPI(void *data) {
	(void)data;
	memset(spi_conf, 0, sizeof(spi_conf));
	memset(spi_sem, 0, sizeof(spi_sem));
	memset(spi_status, 0, sizeof(spi_status));

	int i;
	for (i = 0; i < PERIPHERAL_NUM(spi); i++) {
		int idx = GET_PERIPHERAL_ID(spi, i);
		spi_sem[idx] = vosSemCreate(1);
	}

#if STM32_SPI_USE_SPI1
	spidrv[PERIPHERAL_ID(1)] = &SPID1;
#endif
#if STM32_SPI_USE_SPI2
	spidrv[PERIPHERAL_ID(2)] = &SPID2;
#endif
#if STM32_SPI_USE_SPI3
	spidrv[PERIPHERAL_ID(3)] = &SPID3;
#endif

	return 0;
}

int vhalSpiInit(uint32_t spi, vhalSpiConf *conf) {
	if (spi >= PERIPHERAL_NUM(spi))
		return -1;
	int spiid = GET_PERIPHERAL_ID(spi, spi);
	/*if (spi_status[spiid]) {
		//already active
		return 0;
	}*/
	SPIConfig *cfg  = &spicfg[spiid];
	spi_conf[spiid] = conf;
	int clk = 1;

	cfg->end_cb = NULL;
	//determine nearest clock speed: page 593 of RM_f401: BR[0:2] in CR1
	//SPI is clocked by apb2 == _system_frequency
	int i, cnt = 2;
	for (i = 0; i < 8; i++, cnt *= 2) {
		if ((_system_frequency / cnt) <= conf->clock) {
			clk = (i << 3);
			conf->clock = (_system_frequency / cnt);
			break;
		}
	}
	if (clk == 1) {
		//clock not supported
		return -1;
	}
	cfg->cr1 = clk;

	//POLARITY & PHASE
	switch (conf->mode) {
		case SPI_MODE_LOW_FIRST:
			cfg->cr1 |= 0;
			break;
		case SPI_MODE_LOW_SECOND:
			cfg->cr1 |= 0 | SPI_CR1_CPHA;
			break;
		case SPI_MODE_HIGH_FIRST:
			cfg->cr1 |= SPI_CR1_CPOL | 0;
			break;
		case SPI_MODE_HIGH_SECOND:
			cfg->cr1 |= SPI_CR1_CPOL | SPI_CR1_CPHA;
			break;
		default:
			return -1;
	}

	//WORD SIZE
	switch (conf->bits) {
		case SPI_BITS_8:
			break;
		case SPI_BITS_16:
			cfg->cr1 |= SPI_CR1_DFF;
			break;
		default:
			return -1;
	}

	//cfg->cr1 = SPI_CR1_CPHA | SPI_CR1_BR_1;
	//PIN CONFIGURATION
	cfg->ssport = CPIN_PORT(conf->nss);
	cfg->sspad = PIN_PAD(conf->nss);

	//set other pins to peripherals: TODO: consider also alternate function 6 (page 45 DS_f401)
	vPinSetModeEx(CPIN_PORT(conf->sclk), PIN_PAD(conf->sclk), PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
	vPinSetModeEx(CPIN_PORT(conf->miso), PIN_PAD(conf->miso), PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
	vPinSetModeEx(CPIN_PORT(conf->mosi), PIN_PAD(conf->mosi), PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
	vPinSetModeEx(CPIN_PORT(conf->nss), PIN_PAD(conf->nss), PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
	//vPinWrite(CPIN_PORT(conf->nss), PIN_PAD(conf->nss),1);
	//master ignored, chibios has only master support
	spiStart(spidrv[spiid], cfg);
	spi_status[spiid] = 1;
	return 0;
}

int vhalSpiLock(uint32_t spi) {
	if (spi >= PERIPHERAL_NUM(spi))
		return -1;
	vosSemWait(spi_sem[GET_PERIPHERAL_ID(spi, spi)]);
	return 0;
}
int vhalSpiUnlock(uint32_t spi) {
	vosSemSignal(spi_sem[GET_PERIPHERAL_ID(spi, spi)]);
	return 0;
}

int vhalSpiSelect(uint32_t spi) {
	spiSelect(spidrv[GET_PERIPHERAL_ID(spi, spi)]);
	return 0;
}
int vhalSpiUnselect(uint32_t spi) {
	spiUnselect(spidrv[GET_PERIPHERAL_ID(spi, spi)]);
	return 0;
}


int vhalSpiExchange(uint32_t spi, void *tosend, void *toread, uint32_t blocks) {
	SPIDriver *drv = spidrv[GET_PERIPHERAL_ID(spi, spi)];
	if (toread == NULL && tosend == NULL) {
		spiIgnore(drv, blocks);
	} else if (toread == NULL) {
		spiSend(drv, blocks, tosend);
	} else if (tosend == NULL) {
		spiReceive(drv, blocks, toread);
	} else {
		spiExchange(drv, blocks, tosend, toread);
	}
	return 0;
}

int vhalSpiDone(uint32_t spi) {
	int spiid = GET_PERIPHERAL_ID(spi, spi);
	if (spi_status[spiid]) {
		spiStop(spidrv[spiid]);
		spi_status[spiid] = 0;
	}
	return 0;
}

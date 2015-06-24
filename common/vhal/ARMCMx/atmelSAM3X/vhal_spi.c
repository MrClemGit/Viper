#include "vhal.h"
#include "vhal_gpio.h"
#include "hal.h"


//smt32f4 has max 6 spi peripherals

static vhalSpiConf *spi_conf[2];
static VSemaphore spi_sem[2];
static uint8_t spi_status[2];
static SPIDriver *spidrv[2];
static SPIConfig spicfg[2];


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

#if SAM3X_SPI_USE_SPI1
	spidrv[PERIPHERAL_ID(1)] = &SPID1;
#endif
#if SAM3X_SPI_USE_SPI2
	spidrv[PERIPHERAL_ID(2)] = &SPID2;
#endif

	return 0;
}

int vhalSpiInit(uint32_t spi, vhalSpiConf *conf) {
	if (spi >= PERIPHERAL_NUM(spi))
		return -1;
	int spiid = GET_PERIPHERAL_ID(spi, spi);
	SPIConfig *cfg  = &spicfg[spiid];
	spi_conf[spiid] = conf;

	cfg->end_cb = NULL;
	//SPI is prescaled by 1 to 255 with respect to main clock
	if ( conf->clock > _system_frequency  || conf->clock < _system_frequency / 255) {
		return -1;
	}
	cfg->clk = conf->clock;

	//POLARITY & PHASE
	switch (conf->mode) {
		case SPI_MODE_LOW_FIRST:
			cfg->trflags = SPI_POL_ZERO | SPI_PHASE_SF;
			break;
		case SPI_MODE_LOW_SECOND:
			cfg->trflags = SPI_POL_ZERO | SPI_PHASE_SS;
			break;
		case SPI_MODE_HIGH_FIRST:
			cfg->trflags = SPI_POL_ONE | SPI_PHASE_SF;
			break;
		case SPI_MODE_HIGH_SECOND:
			cfg->trflags = SPI_POL_ONE | SPI_PHASE_SS;
			break;
		default:
			return -1;
	}

	//WORD SIZE
	switch (conf->bits) {
		case SPI_BITS_8:
			cfg->trflags |= SAM3X_SPI_BITS_8;
			break;
		case SPI_BITS_16:
			cfg->trflags |= SAM3X_SPI_BITS_16;
			break;
		default:
			return -1;
	}

	//PIN CONFIGURATION
	cfg->port = CPIN_PORT(conf->nss);
	cfg->pad = PIN_PAD(conf->nss);
	//Low level cfg
	cfg->rxch = SAM3X_SPI1_DMA_RX;
	cfg->txch = SAM3X_SPI1_DMA_TX;
	cfg->btsdelay=0;
	cfg->bcsdelay=1;

	//set other pins to peripherals: TODO: consider also alternate function 6 (page 45 DS_f401)
	vhalPinSetToPeripheral(conf->sclk, PRPH_SER, SAM3X_PIN_PR(conf->sclk));
	vhalPinSetToPeripheral(conf->miso, PRPH_SER, SAM3X_PIN_PR(conf->miso));
	vhalPinSetToPeripheral(conf->mosi, PRPH_SER, SAM3X_PIN_PR(conf->mosi));
	vhalPinSetToPeripheral(conf->nss, PRPH_SER, PAL_MODE_OUTPUT_PUSHPULL);
	//vPinSetModeEx(CPIN_PORT(conf->sclk), PIN_PAD(conf->sclk), PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
	//vPinSetModeEx(CPIN_PORT(conf->miso), PIN_PAD(conf->miso), PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
	//vPinSetModeEx(CPIN_PORT(conf->mosi), PIN_PAD(conf->mosi), PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
	//vPinSetModeEx(CPIN_PORT(conf->nss), PIN_PAD(conf->nss), PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
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

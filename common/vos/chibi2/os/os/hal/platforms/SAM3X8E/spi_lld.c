/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    templates/spi_lld.c
 * @brief   SPI Driver subsystem low level driver source template.
 *
 * @addtogroup SPI
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_SPI || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   SPI1 driver identifier.
 */
#if SAM3X_SPI_USE_SPI1 || defined(__DOXYGEN__)
SPIDriver SPID1;
#endif

#if SAM3X_SPI_USE_SPI2 || defined(__DOXYGEN__)
SPIDriver SPID2;
#endif


/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level SPI driver initialization.
 *
 * @notapi
 */
void spi_lld_init(void) {

#if SAM3X_SPI_USE_SPI1
    /* Driver initialization.*/
    spiObjectInit(&SPID1);
    SPID1.spi = SPI0;
#endif

#if SAM3X_SPI_USE_SPI2
    /* Driver initialization.*/
    spiObjectInit(&SPID2);
    SPID2.spi = SPI1;
#endif

}

void spi_configure(spidef_t spi, const SPIConfig *cfg) {
    if (cfg->clk) {
        /* master mode */
        spi_set_master_mode(spi);
        spi_disable_mode_fault_detect(spi);
        spi_disable_loopback(spi);

/*
        switch (cfg->nss) {
            case 0: palSetPadMode(IOPORTA, 28, PAL_MODE_OUTPUT_PUSHPULL); break;
            case 1: palSetPadMode(IOPORTA, 29, PAD_TO_A); break;
            case 2: palSetPadMode(IOPORTB, 21, PAD_TO_B); break;
        }
*/

        //palSetPadMode(IOPORTA, 25, PAD_TO_A); /*MISO*/
        //palSetPadMode(IOPORTA, 26, PAD_TO_A); /*MOSI*/
        //palSetPadMode(IOPORTA, 27, PAD_TO_A); /*SCK*/

        /* fixed peripheral: the cs pin is managed by gpio */
        spi_set_fixed_peripheral_select(spi);
        spi_set_peripheral_chip_select_value(spi, spi_get_pcs(0));
        spi_disable_peripheral_select_decode(spi);
        spi_set_delay_between_chip_select(spi, 1);

        //nss param is at zero: we support only PA:28 as the cs pin
        spi_set_transfer_delay(spi, 0, cfg->bcsdelay, cfg->btsdelay);
        spi_set_bits_per_transfer(spi, 0, cfg->trflags & 0xfffffff0);
        spi_set_baudrate_div(spi, 0, cfg->clk);
        spi_set_clock_polarity(spi, 0, cfg->trflags & SPI_POL_ONE);
        spi->SPI_CSR[0] |= SPI_CSR_CSAAT;
        spi_set_clock_phase(spi, 0, cfg->trflags & SPI_PHASE_SS);

    } else {
        /* slave mode */
        /* TODO: not supported yet*/
    }
}




static void dma_rx_callback(uint32_t channel, uint32_t err, void *arg) {
    (void)channel;
    SPIDriver *spip = (SPIDriver*)arg;
    if (err) {
        SAM3X_SPI_DMA_ERROR_HOOK(spip);
    }
    dmaStop(spip->config->rxch);
    _spi_isr_code(spip);

}


static void dma_tx_callback(uint32_t channel, uint32_t err, void *arg) {
    (void)channel;
    SPIDriver *spip = (SPIDriver*)arg;
    if (err)
        SAM3X_SPI_DMA_ERROR_HOOK(spip);
    while ((spip->spi->SPI_SR & SPI_SR_TXEMPTY) == 0);
    dmaStop(spip->config->txch);
}


/**
 * @brief   Configures and activates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_start(SPIDriver *spip) {

    if (spip->state == SPI_STOP) {
        /* Enables the peripheral.*/
#if SAM3X_SPI_USE_SPI1
        if (spip == &SPID1)
            pmc_enable_periph_clk(ID_SPI0);
#endif
        spi_disable(spip->spi);
        spi_reset(spip->spi);
        spi_reset(spip->spi);
        spi_configure(spip->spi, spip->config);

        if (spip->config->dma_active) {
            DMA_MPCFG_SETUP(&spip->dmatx, 1);
            DMA_PMCFG_SETUP(&spip->dmarx, 2);
            dmaEnsureStop(spip->config->rxch);
            dmaEnsureStop(spip->config->txch);
            dmaChannelActivate(spip->config->rxch, dma_rx_callback, (void *)spip, &spip->dmarx);
            dmaChannelActivate(spip->config->txch, dma_tx_callback, (void *)spip, &spip->dmatx);
        }
        spi_enable(spip->spi);

    }
    /* Configures the peripheral.*/
}

/**
 * @brief   Deactivates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_stop(SPIDriver *spip) {

    if (spip->state == SPI_READY) {
        /* Resets the peripheral.*/

        spi_reset(spip->spi);
        spi_disable(spip->spi);
        /* Disables the peripheral.*/
#if SAM3X_SPI_USE_SPI1
        if (spip == &SPID1)
            pmc_disable_periph_clk(ID_SPI0);
#endif
#if SAM3X_SPI_USE_SPI2
        if (spip == &SPID2)
            pmc_disable_periph_clk(ID_SPI1);
#endif

        if (spip->config->dma_active) {
            dmaChannelDeactivate(spip->config->rxch);
            dmaChannelDeactivate(spip->config->txch);
        }

    }
}

/**
 * @brief   Asserts the slave select signal and prepares for transfers.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_select(SPIDriver *spip) {
    /*switch (spip->config->nss) {
        case 0: palClearPad(IOPORTA, 28); break;
        case 1: palClearPad(IOPORTA, 29); break;
        case 2: palClearPad(IOPORTB, 21); break;
    }*/
        palClearPad((ioportid_t)spip->config->port,spip->config->pad);
}

/**
 * @brief   Deasserts the slave select signal.
 * @details The previously selected peripheral is unselected.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_unselect(SPIDriver *spip) {
    /*switch (spip->config->nss) {
        case 0: palSetPad(IOPORTA, 28); break;
        case 1: palSetPad(IOPORTA, 29); break;
        case 2: palSetPad(IOPORTB, 21); break;
    }*/
    palSetPad((ioportid_t)spip->config->port,spip->config->pad);
}


uint16_t spi_transfer_one(spidef_t spi, uint8_t val) {
    uint32_t d;

    d = SPI_TDR_PCS(0) | SPI_TDR_TD(val) ;
    while ((spi->SPI_SR & SPI_SR_TDRE) == 0);
    spi->SPI_TDR = d;
    while ((spi->SPI_SR & SPI_SR_RDRF) == 0);
    return spi->SPI_RDR & 0xffff;
}

void spi_transfer(spidef_t spi, uint8_t *rbuf, uint8_t *tbuf, int len, int incrf, int incrt) {
    int i;


    for (i = 0; i < len; i++ ) {
        *rbuf = spi_transfer_one(spi, *tbuf);
        if (incrf) rbuf++;
        if (incrt) tbuf++;
    }
}

void spiTransfer(SPIDriver *spip, size_t n, uint8_t *rbuf, uint8_t *tbuf, int incrf, int incrt) {
    chSysLock();
    spi_transfer(spip->spi, rbuf, tbuf, n, incrf, incrt);
    chSysUnlock();
}

/**
 * @brief   Ignores data on the SPI bus.
 * @details This asynchronous function starts the transmission of a series of
 *          idle words on the SPI bus and ignores the received data.
 * @post    At the end of the operation the configured callback is invoked.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to be ignored
 *
 * @notapi
 */
void spi_lld_ignore(SPIDriver *spip, size_t n) {

    uint32_t nilbuf1 = 0;
    uint32_t nilbuf2 = 0;

    dmaEnsureStop(spip->config->rxch);
    dmaEnsureStop(spip->config->txch);
    DMA_M2P_TRANSFER_EXT(&spip->dmatx, n, &nilbuf1, spi_get_tx_access(spip->spi), DMA_FIXED, DMA_FIXED);
    DMA_P2M_TRANSFER_EXT(&spip->dmarx, n, spi_get_rx_access(spip->spi), &nilbuf2, DMA_FIXED, DMA_FIXED);
    dmaSetupTransfer(spip->config->txch, &spip->dmatx);
    dmaSetupTransfer(spip->config->rxch, &spip->dmarx);
    dmaStart(spip->config->rxch);
    dmaStart(spip->config->txch);

}

/**
 * @brief   Exchanges data on the SPI bus.
 * @details This asynchronous function starts a simultaneous transmit/receive
 *          operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to be exchanged
 * @param[in] txbuf     the pointer to the transmit buffer
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @notapi
 */
void spi_lld_exchange(SPIDriver *spip, size_t n,
                      const void *txbuf, void *rxbuf) {

    dmaEnsureStop(spip->config->rxch);
    dmaEnsureStop(spip->config->txch);
    DMA_M2P_TRANSFER_EXT(&spip->dmatx, n, txbuf, spi_get_tx_access(spip->spi), DMA_INCR, DMA_FIXED);
    DMA_P2M_TRANSFER_EXT(&spip->dmarx, n, spi_get_rx_access(spip->spi), rxbuf, DMA_FIXED, DMA_INCR);
    dmaSetupTransfer(spip->config->txch, &spip->dmatx);
    dmaSetupTransfer(spip->config->rxch, &spip->dmarx);
    dmaStart(spip->config->rxch);
    dmaStart(spip->config->txch);

}

/**
 * @brief   Sends data over the SPI bus.
 * @details This asynchronous function starts a transmit operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to send
 * @param[in] txbuf     the pointer to the transmit buffer
 *
 * @notapi
 */
void spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf) {

    uint32_t nilbuf1 = 0;

    dmaEnsureStop(spip->config->rxch);
    dmaEnsureStop(spip->config->txch);
    DMA_M2P_TRANSFER_EXT(&spip->dmatx, n, txbuf, spi_get_tx_access(spip->spi), DMA_INCR, DMA_FIXED);
    DMA_P2M_TRANSFER_EXT(&spip->dmarx, n, spi_get_rx_access(spip->spi), &nilbuf1, DMA_FIXED, DMA_FIXED);
    dmaSetupTransfer(spip->config->txch, &spip->dmatx);
    dmaSetupTransfer(spip->config->rxch, &spip->dmarx);
    dmaStart(spip->config->rxch);
    dmaStart(spip->config->txch);

}

/**
 * @brief   Receives data from the SPI bus.
 * @details This asynchronous function starts a receive operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to receive
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @notapi
 */
void spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf) {

    uint32_t nilbuf1 = 0;

    dmaEnsureStop(spip->config->rxch);
    dmaEnsureStop(spip->config->txch);
    DMA_M2P_TRANSFER_EXT(&spip->dmatx, n, &nilbuf1, spi_get_tx_access(spip->spi), DMA_FIXED, DMA_FIXED);
    DMA_P2M_TRANSFER_EXT(&spip->dmarx, n, spi_get_rx_access(spip->spi), rxbuf, DMA_FIXED, DMA_INCR);
    dmaSetupTransfer(spip->config->txch, &spip->dmatx);
    dmaSetupTransfer(spip->config->rxch, &spip->dmarx);
    dmaStart(spip->config->rxch);
    dmaStart(spip->config->txch);

}

/**
 * @brief   Exchanges one frame using a polled wait.
 * @details This synchronous function exchanges one frame using a polled
 *          synchronization method. This function is useful when exchanging
 *          small amount of data on high speed channels, usually in this
 *          situation is much more efficient just wait for completion using
 *          polling than suspending the thread waiting for an interrupt.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] frame     the data frame to send over the SPI bus
 * @return              The received data frame from the SPI bus.
 */
uint16_t spi_lld_polled_exchange(SPIDriver *spip, uint16_t frame) {

    spi_put(spip->spi, frame);
    while (!spi_is_tx_empty(spip->spi));
    return spi_get(spip->spi);
}

#endif /* HAL_USE_SPI */

/** @} */

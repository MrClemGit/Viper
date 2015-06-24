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
 * @file    templates/spi_lld.h
 * @brief   SPI Driver subsystem low level driver header template.
 *
 * @addtogroup SPI
 * @{
 */

#ifndef _SPI_LLD_H_
#define _SPI_LLD_H_

#if HAL_USE_SPI || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   SPI driver enable switch.
 * @details If set to @p TRUE the support for SPI1 is included.
 */
#if !defined(SAM3X_SPI_USE_SPI1) || defined(__DOXYGEN__)
#define SAM3X_SPI_USE_SPI1               FALSE
#endif

#if !defined(SAM3X_SPI_USE_SPI2) || defined(__DOXYGEN__)
#define SAM3X_SPI_USE_SPI2               FALSE
#endif

/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/


typedef Spi *spidef_t;

/**
 * @brief   Type of a structure representing an SPI driver.
 */
typedef struct SPIDriver SPIDriver;

/**
 * @brief   SPI notification callback type.
 *
 * @param[in] spip      pointer to the @p SPIDriver object triggering the
 *                      callback
 */
typedef void (*spicallback_t)(SPIDriver *spip);

/**
 * @brief   Driver configuration structure.
 * @note    Implementations may extend this structure to contain more,
 *          architecture dependent, fields.
 */
typedef struct {
    /**
     * @brief Operation complete callback.
     */
    spicallback_t         end_cb;
    /* End of the mandatory fields.*/

    uint8_t dma_active;
    uint8_t rxch;
    uint8_t txch;
    uint8_t pad;
    uint8_t clk;
    uint8_t trflags;
    uint8_t btsdelay;
    uint8_t bcsdelay;
    void * port;


} SPIConfig;


#define SPI_POL_ZERO  0
#define SPI_POL_ONE   1

/*sample first edge */
#define SPI_PHASE_SF  0
/*sample second edge */
#define SPI_PHASE_SS  2

/* PA28, PA29, PB21 */
#define SPI_NSS_0 0
#define SPI_NSS_1 1
#define SPI_NSS_2 2

#define SAM3X_SPI_BITS_8  SPI_CSR_BITS_8_BIT
#define SAM3X_SPI_BITS_16  SPI_CSR_BITS_16_BIT


#define SPI_SETUP_NSS(cfg,nssp) ((cfg)->nss = (nssp))
#define SPI_SETUP_DMA(cfg,rx,tx) ((cfg)->dma_active=1,(cfg)->rxch=(rx),(cfg)->txch=(tx))


#define SPI_SETUP_AS_MASTER(cfg)  (cfg)->clk = 1
#define SPI_SETUP_MASTER_CLK(cfg, clock) (cfg)->clk = (clock)
#define SPI_SETUP_TRANSFER(cfg,pol,ph,bits) (cfg)->trflags = (pol) | (ph) | (bits)
#define SPI_SETUP_BAUDRATE(cfg,baud) (cfg)->clk = SystemCoreClock/(baud)

#define SPI_SETUP_DELAYS(cfg,bts,bcs) ((cfg)->btsdelay=(bts),(cfg)->bcsdelay=(bcs))

#define SPI_SETUP_AS_SLAVE(cfg)  (cfg)->clk = 0




/**
 * @brief   Structure representing an SPI driver.
 * @note    Implementations may extend this structure to contain more,
 *          architecture dependent, fields.
 */
struct SPIDriver {
    /**
     * @brief Driver state.
     */
    spistate_t            state;
    /**
     * @brief Current configuration data.
     */
    const SPIConfig       *config;
#if SPI_USE_WAIT || defined(__DOXYGEN__)
    /**
     * @brief Waiting thread.
     */
    Thread                *thread;
#endif /* SPI_USE_WAIT */
#if SPI_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
    /**
     * @brief Mutex protecting the bus.
     */
    Mutex                 mutex;
#elif CH_USE_SEMAPHORES
    Semaphore             semaphore;
#endif
#endif /* SPI_USE_MUTUAL_EXCLUSION */
#if defined(SPI_DRIVER_EXT_FIELDS)
    SPI_DRIVER_EXT_FIELDS
#endif
    /* End of the mandatory fields.*/

    /* registers */
    spidef_t spi;
    dmatrans_t dmatx;
    dmatrans_t dmarx;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/



/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if SAM3X_SPI_USE_SPI1 && !defined(__DOXYGEN__)
extern SPIDriver SPID1;
#endif

#if SAM3X_SPI_USE_SPI2 && !defined(__DOXYGEN__)
extern SPIDriver SPID2;
#endif



/** Status codes used by the SPI driver. */
typedef enum {
    SPI_ERROR = -1,
    SPI_OK = 0,
    SPI_ERROR_TIMEOUT = 1,
    SPI_ERROR_ARGUMENT,
    SPI_ERROR_OVERRUN,
    SPI_ERROR_MODE_FAULT,
    SPI_ERROR_OVERRUN_AND_MODE_FAULT
} spi_status_t;

/** SPI Chip Select behavior modes while transferring. */
typedef enum spi_cs_behavior {
    /** CS does not rise until a new transfer is requested on different chip select. */
    SPI_CS_KEEP_LOW = SPI_CSR_CSAAT,
    /** CS rises if there is no more data to transfer. */
    SPI_CS_RISE_NO_TX = 0,
    /** CS is de-asserted systematically during a time DLYBCS. */
    SPI_CS_RISE_FORCED = SPI_CSR_CSNAAT
} spi_cs_behavior_t;

/**
 * \brief Generate Peripheral Chip Select Value from Chip Select ID
 * \note When chip select n is working, PCS bit n is set to low level.
 *
 * \param chip_sel_id The chip select number used
 */
#define spi_get_pcs(chip_sel_id) ((~(1u<<(chip_sel_id)))&0xF)

/**
 * \brief Reset SPI and set it to Slave mode.
 *
 * \param p_spi Pointer to an SPI instance.
 */
#define spi_reset(_spi) (_spi)->SPI_CR = SPI_CR_SWRST

/**
 * \brief Enable SPI.
 *
 * \param p_spi Pointer to an SPI instance.
 */
#define spi_enable(_spi) (_spi)->SPI_CR = SPI_CR_SPIEN

/**
 * \brief Disable SPI.
 *
 * \note CS is de-asserted, which indicates that the last data is done, and user
 * should check TX_EMPTY before disabling SPI.
 *
 * \param p_spi Pointer to an SPI instance.
 */
#define spi_disable(_spi) (_spi)->SPI_CR = SPI_CR_SPIDIS

/**
 * \brief Issue a LASTXFER command.
 *  The next transfer is the last transfer and after that CS is de-asserted.
 *
 * \param p_spi Pointer to an SPI instance.
 */
#define spi_set_lastxfer(_spi) (_spi)->SPI_CR = SPI_CR_LASTXFER

/**
 * \brief Set SPI to Master mode.
 *
 * \param p_spi Pointer to an SPI instance.
 */
#define spi_set_master_mode(_spi) (_spi)->SPI_MR |= SPI_MR_MSTR


/**
 * \brief Set SPI to Slave mode.
 *
 * \param p_spi Pointer to an SPI instance.
 */
#define spi_set_slave_mode(_spi) (_spi)->SPI_MR &= (~SPI_MR_MSTR)

/**
 * \brief Get SPI work mode.
 *
 * \param p_spi Pointer to an SPI instance.
 *
 * \return 1 for master mode, 0 for slave mode.
 */
#define spi_get_mode(_spi) (((_spi)->SPI_MR & SPI_MR_MSTR) ? 1:0)


//#define spi_set_peripheral_chip_select_value(_spi,ul_value) ((_spi)->SPI_MR &= (~SPI_MR_PCS_Msk), (_spi)->SPI_MR |= SPI_MR_PCS(ul_value))


/**
 * \brief Set Variable Peripheral Select.
 * Peripheral Chip Select can be controlled by SPI_TDR.
 *
 * \param p_spi Pointer to an SPI instance.
 */
#define spi_set_variable_peripheral_select(_spi) (_spi)->SPI_MR |= SPI_MR_PS

/**
 * \brief Set Fixed Peripheral Select.
 *  Peripheral Chip Select is controlled by SPI_MR.
 *
 * \param p_spi Pointer to an SPI instance.
 */
#define spi_set_fixed_peripheral_select(_spi) (_spi)->SPI_MR &= (~SPI_MR_PS)


/**
 * \brief Get Peripheral Select mode.
 *
 * \param p_spi Pointer to an SPI instance.
 *
 * \return 1 for Variable mode, 0 for fixed mode.
 */
#define spi_get_peripheral_select_mode(_spi) (((_spi)->SPI_MR & SPI_MR_PS) ? 1:0)

/**
 * \brief Enable Peripheral Select Decode.
 *
 * \param p_spi Pointer to an SPI instance.
 */
#define spi_enable_peripheral_select_decode(_spi) (_spi)->SPI_MR |= SPI_MR_PCSDEC

/**
 * \brief Disable Peripheral Select Decode.
 *
 * \param p_spi Pointer to an SPI instance.
 */
#define spi_disable_peripheral_select_decode(_spi) (_spi)->SPI_MR &= (~SPI_MR_PCSDEC)

/**
 * \brief Get Peripheral Select Decode mode.
 *
 * \param p_spi Pointer to an SPI instance.
 *
 * \return 1 for decode mode, 0 for direct mode.
 */
#define spi_get_peripheral_select_decode_setting(_spi) (((_spi)->SPI_MR & SPI_MR_PCSDEC) ? 1:0)

/**
 * \brief Enable Mode Fault Detection.
 *
 * \param p_spi Pointer to an SPI instance.
 */
#define spi_enable_mode_fault_detect(_spi) (_spi)->SPI_MR &= (~SPI_MR_MODFDIS)


/**
 * \brief Disable Mode Fault Detection.
 *
 * \param p_spi Pointer to an SPI instance.
 */
#define spi_disable_mode_fault_detect(_spi) (_spi)->SPI_MR |= SPI_MR_MODFDIS


/**
 * \brief Check if mode fault detection is enabled.
 *
 * \param p_spi Pointer to an SPI instance.
 *
 * \return 1 for disabled, 0 for enabled.
 */
#define spi_get_mode_fault_detect_setting(_spi) (((_spi)->SPI_MR & SPI_MR_MODFDIS) ?1:0)

/**
 * \brief Enable waiting RX_EMPTY before transfer starts.
 *
 * \param p_spi Pointer to an SPI instance.
 */
#define spi_enable_tx_on_rx_3(_spi) (_spi)->SPI_MR |= SPI_MR_WDRBT

/**
 * \brief Disable waiting RX_EMPTY before transfer starts.
 *
 * \param p_spi Pointer to an SPI instance.
 */
#define spi_disable_tx_on_rx_empty(_spi) (_spi)->SPI_MR &= (~SPI_MR_WDRBT)

/**
 * \brief Check if SPI waits RX_EMPTY before transfer starts.
 *
 * \param p_spi Pointer to an SPI instance.
 *
 * \return 1 for SPI waits, 0 for no wait.
 */
#define spi_get_tx_on_rx_empty_setting(_spi) (((_spi)->SPI_MR & SPI_MR_WDRBT) ? 1:0)

/**
 * \brief Enable loopback mode.
 *
 * \param p_spi Pointer to an SPI instance.
 */
#define spi_enable_loopback(_spi) (_spi)->SPI_MR |= SPI_MR_LLB

/**
 * \brief Disable loopback mode.
 *
 * \param p_spi Pointer to an SPI instance.
 */
#define spi_disable_loopback(_spi) (_spi)->SPI_MR &= (~SPI_MR_LLB)


/**
 * \brief Read status register.
 *
 * \param p_spi Pointer to an SPI instance.
 *
 * \return SPI status register value.
 */
#define spi_read_status(_spi) (_spi)->SPI_SR


/**
 * \brief Test if the SPI is enabled.
 *
 * \param p_spi Pointer to an SPI instance.
 *
 * \return 1 if the SPI is enabled, otherwise 0.
 */
#define spi_is_enabled(_spi) (((_spi)->SPI_SR & SPI_SR_SPIENS) ? 1:0)

/**
 * \brief Put one data to a SPI peripheral.
 *
 * \param p_spi Base address of the SPI instance.
 * \param data The data byte to be loaded
 *
 */
#define spi_put(_spi, data) (_spi)->SPI_TDR = SPI_TDR_TD(data)


/** \brief Get one data to a SPI peripheral.
 *
 * \param p_spi Base address of the SPI instance.
 * \return The data byte
 *
 */
#define spi_get(_spi) (_spi)->SPI_RDR & SPI_RDR_RD_Msk


/**
 * \brief Check if all transmissions are complete.
 *
 * \param p_spi Pointer to an SPI instance.
 *
 * \retval 1 if transmissions are complete.
 * \retval 0 if transmissions are not complete.
 */
#define spi_is_tx_empty(_spi) (((_spi)->SPI_SR & SPI_SR_TXEMPTY) ?1:0)

/**
 * \brief Check if all transmissions are ready.
 *
 * \param p_spi Pointer to an SPI instance.
 *
 * \retval 1 if transmissions are complete.
 * \retval 0 if transmissions are not complete.
 */
#define spi_is_tx_ready(_spi) (((_spi)->SPI_SR & SPI_SR_TDRE) ?1:0)

/**
 * \brief Check if the SPI contains a received character.
 *
 * \param p_spi Pointer to an SPI instance.
 *
 * \return 1 if the SPI Receive Holding Register is full, otherwise 0.
 */
#define spi_is_rx_full(_spi) (((_spi)->SPI_SR & SPI_SR_RDRF) ?1:0)

/**
 * \brief Check if all receptions are ready.
 *
 * \param p_spi Pointer to an SPI instance.
 *
 * \return 1 if the SPI Receiver is ready, otherwise 0.
 */
#define spi_is_rx_ready(_spi) ((((_spi)->SPI_SR & (SPI_SR_RDRF | SPI_SR_TXEMPTY)) == (SPI_SR_RDRF | SPI_SR_TXEMPTY)) ? 1:0)

/**
 * \brief Enable SPI interrupts.
 *
 * \param p_spi Pointer to an SPI instance.
 * \param ul_sources Interrupts to be enabled.
 */
#define spi_enable_interrupt(_spi, sources) (_spi)->SPI_IER = sources


/**
 * \brief Disable SPI interrupts.
 *
 * \param p_spi Pointer to an SPI instance.
 * \param ul_sources Interrupts to be disabled.
 */
#define spi_disable_interrupt(_spi, sources) (_spi)->SPI_IDR = sources


/**
 * \brief Read SPI interrupt mask.
 *
 * \param p_spi Pointer to an SPI instance.
 *
 * \return The interrupt mask value.
 */
#define spi_read_interrupt_mask(_spi) (_spi)->SPI_IMR


/**
 * \brief Get transmit data register address for DMA operation.
 *
 * \param p_spi Pointer to an SPI instance.
 *
 * \return Transmit address for DMA access.
 */
#define spi_get_tx_access(_spi) ((void *) & ((_spi)->SPI_TDR))

#define spi_get_rx_access(_spi) ((void *) & ((_spi)->SPI_RDR))



/**
 * \brief Set clock default state.
 *
 * \param p_spi Pointer to an SPI instance.
 * \param ul_pcs_ch Peripheral Chip Select channel (0~3).
 * \param ul_polarity Default clock state is logical one(high)/zero(low).
 */
#define spi_set_clock_polarity(_spi, nss, polarity) ((polarity) ? ((_spi)->SPI_CSR[nss] |= SPI_CSR_CPOL):((_spi)->SPI_CSR[nss] &= (~SPI_CSR_CPOL)))

/**
 * \brief Set Data Capture Phase.
 *
 * \param p_spi Pointer to an SPI instance.
 *  \param ul_pcs_ch Peripheral Chip Select channel (0~3).
 *  \param ul_phase Data capture on the rising/falling edge of clock.
 */
#define spi_set_clock_phase(_spi, nss, phase) ((phase) ? ((_spi)->SPI_CSR[nss] |= SPI_CSR_NCPHA):((_spi)->SPI_CSR[nss] &= (~SPI_CSR_NCPHA)))

/**
 * \brief Configure CS behavior for SPI transfer (\ref spi_cs_behavior_t).
 *
 * \param p_spi Pointer to an SPI instance.
 * \param ul_pcs_ch Peripheral Chip Select channel (0~3).
 * \param ul_cs_behavior Behavior of the Chip Select after transfer.
 */
 /*
void spi_configure_cs_behavior(Spi *p_spi, uint32_t ul_pcs_ch,
                               uint32_t ul_cs_behavior) {
    if (ul_cs_behavior == SPI_CS_RISE_FORCED) {
        p_spi->SPI_CSR[ul_pcs_ch] &= (~SPI_CSR_CSAAT);
        p_spi->SPI_CSR[ul_pcs_ch] |= SPI_CSR_CSNAAT;
    } else if (ul_cs_behavior == SPI_CS_RISE_NO_TX) {
        p_spi->SPI_CSR[ul_pcs_ch] &= (~SPI_CSR_CSAAT);
        p_spi->SPI_CSR[ul_pcs_ch] &= (~SPI_CSR_CSNAAT);
    } else if (ul_cs_behavior == SPI_CS_KEEP_LOW) {
        p_spi->SPI_CSR[ul_pcs_ch] |= SPI_CSR_CSAAT;
    }
}
*/

/**
 * \brief Set number of bits per transfer.
 *
 * \param p_spi Pointer to an SPI instance.
 * \param ul_pcs_ch Peripheral Chip Select channel (0~3).
 * \param ul_bits Number of bits (8~16), use the pattern defined
 *        in the device header file.
 */
#define spi_set_bits_per_transfer(_spi, nss, bits) ((_spi)->SPI_CSR[nss] &= (~SPI_CSR_BITS_Msk),(_spi)->SPI_CSR[nss] |= (bits))


/**
 * \brief Set Serial Clock Baud Rate divider value (SCBR).
 *
 * \param p_spi Pointer to an SPI instance.
 * \param ul_pcs_ch Peripheral Chip Select channel (0~3).
 * \param uc_baudrate_divider Baudrate divider from MCK.
 */
#define spi_set_baudrate_div(_spi, nss, clk) ((_spi)->SPI_CSR[nss] &= (~SPI_CSR_SCBR_Msk), (_spi)->SPI_CSR[nss] |= SPI_CSR_SCBR(clk))

/**
 * \brief Configure timing for SPI transfer.
 *
 * \param p_spi Pointer to an SPI instance.
 * \param ul_pcs_ch Peripheral Chip Select channel (0~3).
 * \param uc_dlybs Delay before SPCK (in number of MCK clocks).
 * \param uc_dlybct Delay between consecutive transfers (in number of MCK clocks).
 */
#define spi_set_transfer_delay(_spi, nss, bcs, bts) ((_spi)->SPI_CSR[nss] &= ~(SPI_CSR_DLYBS_Msk | SPI_CSR_DLYBCT_Msk), (_spi)->SPI_CSR[nss] |= SPI_CSR_DLYBS(bcs) | SPI_CSR_DLYBCT(bts))



/**
 * \brief Set Peripheral Chip Select (PCS) value.
 *
 * \param p_spi Pointer to an SPI instance.
 * \param ul_value Peripheral Chip Select value.
 *                 If PCS decode mode is not used, use \ref spi_get_pcs to build
 *                 the value to use.
 *                 On reset the decode mode is not enabled.
 *                 The decode mode can be enabled/disabled by follow functions:
 *                 \ref spi_enable_peripheral_select_decode,
 *                 \ref spi_disable_peripheral_select_decode.
 */
#define spi_set_peripheral_chip_select_value(_spi, value) ((_spi)->SPI_MR &= (~SPI_MR_PCS_Msk), (_spi)->SPI_MR |= SPI_MR_PCS(value))

/**
 * \brief Set delay between chip selects (in number of MCK clocks).
 *  If DLYBCS <= 6, 6 MCK clocks will be inserted by default.
 *
 * \param p_spi Pointer to an SPI instance.
 * \param ul_delay Delay between chip selects (in number of MCK clocks).
 */
#define spi_set_delay_between_chip_select(_spi, delay) ((_spi)->SPI_MR &= (~SPI_MR_DLYBCS_Msk), (_spi)->SPI_MR |= SPI_MR_DLYBCS(delay))



#ifdef __cplusplus
extern "C" {
#endif
void spi_lld_init(void);
void spi_lld_start(SPIDriver *spip);
void spi_lld_stop(SPIDriver *spip);
void spi_lld_select(SPIDriver *spip);
void spi_lld_unselect(SPIDriver *spip);
void spi_lld_ignore(SPIDriver *spip, size_t n);
void spi_lld_exchange(SPIDriver *spip, size_t n,
                      const void *txbuf, void *rxbuf);
void spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf);
void spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf);
uint16_t spi_lld_polled_exchange(SPIDriver *spip, uint16_t frame);

void spiTransfer(SPIDriver *spip, size_t n, uint8_t *rbuf, uint8_t *tbuf, int incrf, int incrt);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SPI */

#endif /* _SPI_LLD_H_ */

/** @} */

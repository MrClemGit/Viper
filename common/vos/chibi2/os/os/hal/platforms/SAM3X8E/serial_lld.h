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
 * @file    templates/serial_lld.h
 * @brief   Serial Driver subsystem low level driver header template.
 *
 * @addtogroup SERIAL
 * @{
 */

#ifndef _SERIAL_LLD_H_
#define _SERIAL_LLD_H_

#if HAL_USE_SERIAL || defined(__DOXYGEN__)

#include "io_channel.h"
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
 * @brief   SD1 driver enable switch.
 * @details If set to @p TRUE the support for SD1 is included.
 */
#if !defined(SAM3X_SERIAL_USE_SD1) || defined(__DOXYGEN__)
#define SAM3X_SERIAL_USE_SD1             FALSE
#endif
#if !defined(SAM3X_SERIAL_USE_SD2) || defined(__DOXYGEN__)
#define SAM3X_SERIAL_USE_SD2             FALSE
#endif
#if !defined(SAM3X_SERIAL_USE_SD3) || defined(__DOXYGEN__)
#define SAM3X_SERIAL_USE_SD3             FALSE
#endif
#if !defined(SAM3X_SERIAL_USE_SD4) || defined(__DOXYGEN__)
#define SAM3X_SERIAL_USE_SD4            FALSE
#endif
#if !defined(SAM3X_SERIAL_USE_SD5) || defined(__DOXYGEN__)
#define SAM3X_SERIAL_USE_SD5            FALSE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/


typedef Usart *usart_t;
typedef Uart *uart_t;

/**
 * @brief   Generic Serial Driver configuration structure.
 * @details An instance of this structure must be passed to @p sdStart()
 *          in order to configure and start a serial driver operations.
 * @note    Implementations may extend this structure to contain more,
 *          architecture dependent, fields.
 */
typedef struct {
    /**
     * @brief Bit rate.
     */
    uint32_t                  speed;
    /* End of the mandatory fields.*/
    uint32_t                  flags; /*paritity, stop_bits, char len */
} SerialConfig;


#define SAM3X_SERIAL_PARITY_EVEN US_MR_PAR_EVEN
#define SAM3X_SERIAL_PARITY_ODD US_MR_PAR_ODD
#define SAM3X_SERIAL_PARITY_NO US_MR_PAR_NO
#define SAM3X_SERIAL_PARITY_SPACE US_MR_PAR_SPACE
#define SAM3X_SERIAL_PARITY_MARK US_MR_PAR_MARK

#define SAM3X_SERIAL_STOP_1    US_MR_NBSTOP_1_BIT
#define SAM3X_SERIAL_STOP_1_5  US_MR_NBSTOP_1_5_BIT
#define SAM3X_SERIAL_STOP_2    US_MR_NBSTOP_2_BIT

#define SAM3X_SERIAL_CHAR_5_BIT US_MR_CHRL_5_BIT
#define SAM3X_SERIAL_CHAR_6_BIT US_MR_CHRL_6_BIT
#define SAM3X_SERIAL_CHAR_7_BIT US_MR_CHRL_7_BIT
#define SAM3X_SERIAL_CHAR_8_BIT US_MR_CHRL_8_BIT

//defined, but they are equal to the serial counterparts
#define SAM3X_UART_PARITY_EVEN UART_MR_PAR_EVEN
#define SAM3X_UART_PARITY_ODD UART_MR_PAR_ODD
#define SAM3X_UART_PARITY_NO UART_MR_PAR_NO
#define SAM3X_UART_PARITY_SPACE UART_MR_PAR_SPACE
#define SAM3X_UART_PARITY_MARK UART_MR_PAR_MARK

//#define LLD_SERIAL_CONFIG(cfg,baud, len, parity, stop) ((cfg)->sc_speed = (baud), (cfg)->flags = (len)|(parity)|(stop))


/**
 * @brief @p SerialDriver specific data.
 */
#define _serial_driver_data                                                 \
    _base_asynchronous_channel_data                                           \
    /* Driver state.*/                                                        \
    sdstate_t                 state;                                          \
    /* Input queue.*/                                                         \
    InputQueue                iqueue;                                         \
    /* Output queue.*/                                                        \
    OutputQueue               oqueue;                                         \
    /* Input circular buffer.*/                                               \
    uint8_t                   ib[SERIAL_BUFFERS_SIZE];                        \
    /* Output circular buffer.*/                                              \
    uint8_t                   ob[SERIAL_BUFFERS_SIZE];                        \
    /* End of the mandatory fields.*/                                         \
    /* Pointer to the USART registers block.*/                                \
    usart_t             u;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * \brief Enable USART transmitter.
 *
 * \param p_usart Pointer to a USART instance.
 */
#define usart_enable_tx(u) (u)->US_CR = US_CR_TXEN

/**
 * \brief Disable USART transmitter.
 *
 * \param p_usart Pointer to a USART instance.
 */
#define usart_disable_tx(u) (u)->US_CR = US_CR_TXDIS

/**
 * \brief Immediately stop and disable USART transmitter.
 *
 * \param p_usart Pointer to a USART instance.
 */
#define usart_reset_tx(u) (u)->US_CR = US_CR_RSTTX | US_CR_TXDIS


/**
 * \brief Configure the transmit timeguard register.
 *
 * \param p_usart Pointer to a USART instance.
 * \param timeguard The value of transmit timeguard.
 */
#define usart_set_tx_timeguard(u, timeguard) (u)->US_TTGR = timeguard


/**
 * \brief Enable USART receiver.
 *
 * \param p_usart Pointer to a USART instance.
 */
#define usart_enable_rx(u)   (u)->US_CR = US_CR_RXEN

/**
 * \brief Disable USART receiver.
 *
 * \param p_usart Pointer to a USART instance.
 */
#define usart_disable_rx(u)   (u)->US_CR = US_CR_RXDIS

/**
 * \brief Immediately stop and disable USART receiver.
 *
 * \param p_usart Pointer to a USART instance.
 */
#define usart_reset_rx(u) (u)->US_CR = US_CR_RSTRX | US_CR_RXDIS

/**
 * \brief Configure the receive timeout register.
 *
 * \param p_usart Pointer to a USART instance.
 * \param timeout The value of receive timeout.
 */
#define usart_set_rx_timeout(u,timeout) (u)->US_RTOR = timeout


/**
 * \brief Enable USART interrupts.
 *
 * \param p_usart Pointer to a USART peripheral.
 * \param ul_sources Interrupt sources bit map.
 */
#define usart_enable_interrupt(u,ul_sources) (u)->US_IER = ul_sources


/**
 * \brief Disable USART interrupts.
 *
 * \param p_usart Pointer to a USART peripheral.
 * \param ul_sources Interrupt sources bit map.
 */
#define usart_disable_interrupt(u, ul_sources) (u)->US_IDR = ul_sources


/**
 * \brief Read USART interrupt mask.
 *
 * \param p_usart Pointer to a USART peripheral.
 *
 * \return The interrupt mask value.
 */
#define usart_get_interrupt_mask(u)   (u)->US_IMR

/**
 * \brief Get current status.
 *
 * \param p_usart Pointer to a USART instance.
 *
 * \return The current USART status.
 */
#define usart_get_status(u)   (u)->US_CSR

/**
 * \brief Reset status bits (PARE, OVER, MANERR, UNRE and PXBRK in US_CSR).
 *
 * \param p_usart Pointer to a USART instance.
 */
#define usart_reset_status(u)   (u)->US_CR = US_CR_RSTSTA

/**
 * \brief Start transmission of a break.
 *
 * \param p_usart Pointer to a USART instance.
 */
#define usart_start_tx_break(u)   (u)->US_CR = US_CR_STTBRK

/**
 * \brief Stop transmission of a break.
 *
 * \param p_usart Pointer to a USART instance.
 */
#define usart_stop_tx_break(u)   (u)->US_CR = US_CR_STPBRK

/**
 * \brief Start waiting for a character before clocking the timeout count.
 * Reset the status bit TIMEOUT in US_CSR.
 *
 * \param p_usart Pointer to a USART instance.
 */
#define usart_start_rx_timeout(u)   (u)->US_CR = US_CR_STTTO


/**
 * \brief Reset the ITERATION in US_CSR when the ISO7816 mode is enabled.
 *
 * \param p_usart Pointer to a USART instance.
 */
#define usart_reset_iterations(u)   (u)->US_CR = US_CR_RSTIT

/**
 * \brief Reset NACK in US_CSR.
 *
 * \param p_usart Pointer to a USART instance.
 */
#define usart_reset_nack(u)   (u)->US_CR = US_CR_RSTNACK

/**
 * \brief Restart the receive timeout.
 *
 * \param p_usart Pointer to a USART instance.
 */
#define usart_restart_rx_timeout(u)   (u)->US_CR = US_CR_RETTO


/**
 * \brief Drive the pin RTS to 0.
 *
 * \param p_usart Pointer to a USART instance.
 */
#define usart_drive_RTS_pin_low(u)   (u)->US_CR = US_CR_RTSEN

/**
 * \brief Drive the pin RTS to 1.
 *
 * \param p_usart Pointer to a USART instance.
 */
#define usart_drive_RTS_pin_high(u)   (u)->US_CR = US_CR_RTSDIS


/**
 * \brief Check if Transmit is Ready.
 * Check if data have been loaded in USART_THR and are waiting to be loaded
 * into the Transmit Shift Register (TSR).
 *
 * \param p_usart Pointer to a USART instance.
 *
 * \retval 1 No data is in the Transmit Holding Register.
 * \retval 0 There is data in the Transmit Holding Register.
 */
#define usart_is_tx_ready(u)   (((u)->US_CSR & US_CSR_TXRDY) > 0)

/**
 * \brief Check if Transmit Holding Register is empty.
 * Check if the last data written in USART_THR have been loaded in TSR and the
 * last data loaded in TSR have been transmitted.
 *
 * \param p_usart Pointer to a USART instance.
 *
 * \retval 1 Transmitter is empty.
 * \retval 0 Transmitter is not empty.
 */
#define usart_is_tx_empty(u)   (((u)->US_CSR & US_CSR_TXEMPTY) > 0)

/**
 * \brief Check if the received data are ready.
 * Check if Data have been received and loaded into USART_RHR.
 *
 * \param p_usart Pointer to a USART instance.
 *
 * \retval 1 Some data has been received.
 * \retval 0 No data has been received.
 */
#define usart_is_rx_ready(u)  (((u)->US_CSR & US_CSR_RXRDY) > 0)

/**
 * \brief Check if one receive buffer is filled.
 *
 * \param p_usart Pointer to a USART instance.
 *
 * \retval 1 Receive is complete.
 * \retval 0 Receive is still pending.
 */
#define usart_is_rx_buf_end(u)  (((u)->US_CSR & US_CSR_ENDRX) > 0)

/**
 * \brief Check if one transmit buffer is empty.
 *
 * \param p_usart Pointer to a USART instance.
 *
 * \retval 1 Transmit is complete.
 * \retval 0 Transmit is still pending.
 */
#define usart_is_tx_buf_end(u)   (((u)->US_CSR & US_CSR_ENDTX) > 0)

/**
 * \brief Check if both receive buffers are full.
 *
 * \param p_usart Pointer to a USART instance.
 *
 * \retval 1 Receive buffers are full.
 * \retval 0 Receive buffers are not full.
 */
#define usart_is_rx_buf_full(u)   (((u)->US_CSR & US_CSR_RXBUFF) > 0)

/**
 * \brief Check if both transmit buffers are empty.
 *
 * \param p_usart Pointer to a USART instance.
 *
 * \retval 1 Transmit buffers are empty.
 * \retval 0 Transmit buffers are not empty.
 */
#define usart_is_tx_buf_empty(u)   (((u)->US_CSR & US_CSR_TXBUFE) > 0)

/**
 * \brief Write to USART Transmit Holding Register.
 *
 * \note Before writing user should check if tx is ready (or empty).
 *
 * \param p_usart Pointer to a USART instance.
 * \param c Data to be sent.
 *
 * \retval 0 on success.
 * \retval 1 on failure.
 */
#define usart_write(u, c) (((u)->US_CSR & US_CSR_TXRDY) ? (((u)->US_THR =US_THR_TXCHR(c),0) ):(1))


/**
 * \brief Read from USART Receive Holding Register.
 *
 * \note Before reading user should check if rx is ready.
 *
 * \param p_usart Pointer to a USART instance.
 * \param c Pointer where the one-byte received data will be stored.
 *
 * \retval 0 on success.
 * \retval 1 if no data is available or errors.
 */
#define usart_read(u,c) (((u)->US_CSR & US_CSR_RXRDY) ? ((  *(c) = (u)->US_RHR & US_RHR_RXCHR_Msk, 0) ):(1))

#define usart_rxchar(u) ((u)->US_RHR & US_RHR_RXCHR_Msk)

#define usart_txchar(u,c) ((u)->US_THR =US_THR_TXCHR(c))

/**
 * \brief Get Transmit address for DMA operation.
 *
 * \param p_usart Pointer to a USART instance.
 *
 * \return Transmit address for DMA access.
 */
#define usart_get_tx_access(u)  ((uint32_t *) & ( (u)->US_THR))

/**
 * \brief Get Receive address for DMA operation.
 *
 * \param p_usart Pointer to a USART instance.
 *
 * \return Receive address for DMA access.
 */
#define usart_get_rx_access(u)  ((uint32_t *) & ( (u)->US_RHR))



/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if SAM3X_SERIAL_USE_SD1 && !defined(__DOXYGEN__)
extern SerialDriver SD1;
#endif

#if SAM3X_SERIAL_USE_SD2 && !defined(__DOXYGEN__)
extern SerialDriver SD2;
#endif

#if SAM3X_SERIAL_USE_SD3 && !defined(__DOXYGEN__)
extern SerialDriver SD3;
#endif

#if SAM3X_SERIAL_USE_SD4 && !defined(__DOXYGEN__)
extern SerialDriver SD4;
#endif


#if SAM3X_SERIAL_USE_SD5 && !defined(__DOXYGEN__)
extern SerialDriver SD5;
#endif


#ifdef __cplusplus
extern "C" {
#endif
void sd_lld_init(void);
void sd_lld_start(SerialDriver *sdp, const SerialConfig *config);
void sd_lld_stop(SerialDriver *sdp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SERIAL */

#endif /* _SERIAL_LLD_H_ */

/** @} */

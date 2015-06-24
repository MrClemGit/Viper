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
 * @file    templates/uart_lld.c
 * @brief   UART Driver subsystem low level driver source template.
 *
 * @addtogroup UART
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_UART || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* UART internal div factor for sampling */
#define UART_MCK_DIV             16
/* Div factor to get the maximum baud rate */
#define UART_MCK_DIV_MIN_FACTOR  1
/* Div factor to get the minimum baud rate */
#define UART_MCK_DIV_MAX_FACTOR  65535


/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   UART1 driver identifier.
 */
#if SAM3X_UART_USE_UART1 || defined(__DOXYGEN__)
UARTDriver UARTD1;
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


static void serve_interrupt(UARTDriver *uartp) {
    uart_t u = uartp->uart;
    uint32_t status = u->UART_SR; /*status*/
    status &= u->UART_IMR;/* & interrupt_mask */

    chSysLockFromIsr();

    /*
    if ((status & UART_SR_TXRDY) || (status & UART_SR_TXEMPTY)) {
        uartp->txn--;
        uartp->txbuf++;
        if (uartp->txn)
            u->UART_THR = uartp->txbuf[0];
        else {
            u->UART_IDR = UART_IDR_TXRDY | UART_IDR_TXEMPTY;
            uartp->txstate = UART_TX_COMPLETE;
            if (uartp->config->txend1_cb != NULL)
                uartp->config->txend1_cb(uartp);
            if (uartp->txstate == UART_TX_COMPLETE)
                uartp->txstate = UART_TX_IDLE;
        }
    }

    */


    if ((status & UART_SR_ENDTX) || (status & UART_SR_TXBUFE)) {
        u->UART_IDR = UART_IDR_ENDTX | UART_IDR_TXBUFE;
        uartp->txstate = UART_TX_COMPLETE;
        if (uartp->config->txend1_cb != NULL)
            uartp->config->txend1_cb(uartp);
        if (uartp->txstate == UART_TX_COMPLETE)
            uartp->txstate = UART_TX_IDLE;
    }

    /*
        if (status & UART_SR_RXRDY) {
            if (uartp->rxstate == UART_RX_IDLE) {
                int c = (u->UART_RHR);
                uartp->rxcirc[uartp->rxhead] = (uint8_t)c;
                if (uartp->config->rxchar_cb != NULL)
                    uartp->config->rxchar_cb(uartp, uartp->rxcirc[uartp->rxhead]);
                uartp->rxhead =  (uartp->rxhead + 1) % SAM3X_UART_BUFSIZE;
                if (uartp->rxelements < SAM3X_UART_BUFSIZE)
                    uartp->rxelements++;
            } else {
                uartp->rxn--;
                uartp->rxbuf[0] = u->UART_RHR;
                uartp->rxbuf++;
                if (!uartp->rxn) {
                    uartp->rxstate = UART_RX_COMPLETE;
                    if (uartp->config->rxend_cb != NULL)
                        uartp->config->rxend_cb(uartp);
                    if (uartp->rxstate == UART_RX_COMPLETE) {
                        uartp->rxstate = UART_RX_IDLE;
                    }
                }
            }
        }
    */

    if (status & UART_SR_RXBUFF) {
        u->UART_RPR = uartp->rxcirc;
        u->UART_RCR = SAM3X_UART_BUFSIZE;
        u->UART_RNPR = NULL;
        u->UART_RNCR = 0;
        u->UART_PTCR = UART_PTCR_RXTEN;
        if (uartp->rxreq) {
            if (uartp->config->rxend_cb != NULL)
                uartp->config->rxend_cb(uartp);
            uartp->rxreq = 0;
        }
    }

    if ((status & UART_SR_OVRE) || (status & UART_SR_FRAME)) {
        u->UART_CR |= UART_CR_RSTSTA;
        if (uartp->config->rxerr_cb)
            uartp->config->rxerr_cb(uartp, status);
    }
    chSysUnlockFromIsr();
}




#if SAM3X_UART_USE_UART1

CH_IRQ_HANDLER(UART_Handler) {

    CH_IRQ_PROLOGUE();

    serve_interrupt(&UARTD1);

    CH_IRQ_EPILOGUE();
}
#endif


/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level UART driver initialization.
 *
 * @notapi
 */
void uart_lld_init(void) {

#if SAM3X_UART_USE_UART1
    /* Driver initialization.*/
    uartObjectInit(&UARTD1);
    UARTD1.uart = UART;

#endif
}

/**
 * @brief   Configures and activates the UART peripheral.
 *
 * @param[in] uartp      pointer to the @p UARTDriver object
 *
 * @notapi
 */
void uart_lld_start(UARTDriver *uartp) {

    if (uartp->state == UART_STOP) {
        /* Enables the peripheral.*/
#if SAM3X_UART_USE_UART1
        if (&UARTD1 == uartp) {
            uint32_t cd = 0;
            uart_t uart = uartp->uart;
            uartp->rxreq = 0;

            pmc_enable_periph_clk(ID_UART);
            palSetPadMode(IOPORTA, 9, PAD_TO_A);
            //palSetPadMode(IOPORTA, 8, PAL_MODE_INPUT);

            /* Reset and disable receiver & transmitter */
            uart->UART_CR = UART_CR_RSTRX | UART_CR_RSTTX | UART_CR_RXDIS |
                            UART_CR_TXDIS;//UART_CR_RSTRX | UART_CR_RSTTX | UART_CR_RXDIS | UART_CR_TXDIS;

            /* Check and configure baudrate */
            /* Asynchronous, no oversampling */
            cd = (SystemCoreClock / uartp->config->baudrate) / UART_MCK_DIV;
            if (cd < UART_MCK_DIV_MIN_FACTOR || cd > UART_MCK_DIV_MAX_FACTOR)
                cd = UART_MCK_DIV_MIN_FACTOR;

            uart->UART_BRGR = cd;
            /* Configure mode */
            uart->UART_MR = UART_MR_PAR_NO | UART_MR_CHMODE_NORMAL;//UART_MR_PAR_NO;//uartp->config->parity;

            /* Disable PDC channel */
            uart->UART_PTCR = UART_PTCR_RXTDIS | UART_PTCR_TXTDIS;

            uart->UART_IDR = 0xFFFFFFFF;
            //uart->UART_IER = UART_IER_OVRE | UART_IER_FRAME | UART_IER_RXRDY;
            uart->UART_IER = UART_IER_OVRE | UART_IER_FRAME | UART_IER_RXBUFF;

            uart->UART_RPR = uartp->rxcirc;
            uart->UART_RCR = SAM3X_UART_BUFSIZE;
            uart->UART_RNPR = NULL;
            uart->UART_RNCR = 0;
            uart->UART_TPR = NULL;
            uart->UART_TCR = 0;
            uart->UART_TNPR = NULL;
            uart->UART_TNCR = 0;


            nvicEnableVector(UART_IRQn, CORTEX_PRIORITY_MASK(SAM3X_UART_IRQ_PRIORITY));
            /* Enable receiver and transmitter */
            uart->UART_PTCR = UART_PTCR_RXTEN | UART_PTCR_TXTEN;
            uart->UART_CR = UART_CR_RXEN | UART_CR_TXEN;
        }
#endif
    }
    /* Configures the peripheral.*/

}

/**
 * @brief   Deactivates the UART peripheral.
 *
 * @param[in] uartp      pointer to the @p UARTDriver object
 *
 * @notapi
 */
void uart_lld_stop(UARTDriver *uartp) {

    if (uartp->state == UART_READY) {
        /* Resets the peripheral.*/
        /* Disables the peripheral.*/
#if SAM3X_UART_USE_UART1
        if (&UARTD1 == uartp) {
            uartp->uart->UART_CR = UART_CR_RXDIS | UART_CR_TXDIS;
            nvicDisableVector(UART_IRQn);
            pmc_disable_periph_clk(ID_UART);
        }
#endif
    }
}

/**
 * @brief   Starts a transmission on the UART peripheral.
 * @note    The buffers are organized as uint8_t arrays for data sizes below
 *          or equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 * @param[in] n         number of data frames to send
 * @param[in] txbuf     the pointer to the transmit buffer
 *
 * @notapi
 */
void uart_lld_start_send(UARTDriver *uartp, size_t n, const void *txbuf) {

    if (n) {
        uartp->uart->UART_TPR = (uint8_t *)txbuf;
        uartp->uart->UART_TCR = n;
        uartp->uart->UART_TNPR = NULL;
        uartp->uart->UART_TNCR = 0;
        uartp->uart->UART_PTCR = /*UART_PTCR_RXTEN |*/ UART_PTCR_TXTEN;
        uartp->uart->UART_IER = UART_IER_ENDTX | UART_IER_TXBUFE;
    } else {
        uartp->txstate = UART_TX_COMPLETE;
        if (uartp->config->txend1_cb != NULL)
            uartp->config->txend1_cb(uartp);
        /* If the callback didn't explicitly change state then the transmitter
           automatically returns to the idle state.*/
        if (uartp->txstate == UART_TX_COMPLETE)
            uartp->txstate = UART_TX_IDLE;

    }

}

/**
 * @brief   Stops any ongoing transmission.
 * @note    Stopping a transmission also suppresses the transmission callbacks.
 *
 * @param[in] uartp      pointer to the @p UARTDriver object
 *
 * @return              The number of data frames not transmitted by the
 *                      stopped transmit operation.
 *
 * @notapi
 */
size_t uart_lld_stop_send(UARTDriver *uartp) {

    (void)uartp;
    return 0;
}


size_t uart_lld_available(UARTDriver *uartp) {
    chSysLock();
    int avl = (SAM3X_UART_BUFSIZE - uartp->uart->UART_RCR);
    chSysUnlock();
    return avl;
}

/*TODO: OPTIMIZE!!!*/
size_t uart_lld_copy(UARTDriver *uartp, uint8_t *buf, uint16_t len) {
    chSysLock();
    memcpy(buf, uartp->rxcirc, len);
    int32_t delta = SAM3X_UART_BUFSIZE-len;
    int32_t toread = uartp->uart->UART_RCR+len;
    if (delta)
        memmove(uartp->rxcirc, uartp->rxcirc + len, delta);
    uartp->uart->UART_RPR = uartp->rxcirc+SAM3X_UART_BUFSIZE-toread;
    uartp->uart->UART_RCR = toread;
    chSysUnlock();
    return len;
}


/**
 * @brief   Starts a receive operation on the UART peripheral.
 * @note    The buffers are organized as uint8_t arrays for data sizes below
 *          or equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 * @param[in] n         number of data frames to send
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @notapi
 */
void uart_lld_start_receive(UARTDriver *uartp, size_t n, void *rxbuf) {
    uartp->rxreq = 1;
    uartp->uart->UART_RPR = (uint8_t *)rxbuf;
    uartp->uart->UART_RCR = 1;
    uartp->uart->UART_PTCR = UART_PTCR_RXTEN /*| UART_PTCR_TXTEN*/;
}

/**
 * @brief   Stops any ongoing receive operation.
 * @note    Stopping a receive operation also suppresses the receive callbacks.
 *
 * @param[in] uartp      pointer to the @p UARTDriver object
 *
 * @return              The number of data frames not received by the
 *                      stopped receive operation.
 *
 * @notapi
 */
size_t uart_lld_stop_receive(UARTDriver *uartp) {

    (void)uartp;
    return 0;
}

#endif /* HAL_USE_UART */

/** @} */

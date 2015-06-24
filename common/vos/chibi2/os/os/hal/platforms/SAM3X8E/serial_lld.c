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
 * @file    templates/serial_lld.c
 * @brief   Serial Driver subsystem low level driver source template.
 *
 * @addtogroup SERIAL
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_SERIAL || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   SD1 driver identifier.
 */
#if SAM3X_SERIAL_USE_SD1 || defined(__DOXYGEN__)
SerialDriver SD1;
#endif

#if SAM3X_SERIAL_USE_SD2 || defined(__DOXYGEN__)
SerialDriver SD2;
#endif

#if SAM3X_SERIAL_USE_SD3 || defined(__DOXYGEN__)
SerialDriver SD3;
#endif

#if SAM3X_SERIAL_USE_SD4 || defined(__DOXYGEN__)
SerialDriver SD4;
#endif

#if SAM3X_SERIAL_USE_SD5 || defined(__DOXYGEN__)
SerialDriver SD5;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver default configuration.
 */
static const SerialConfig default_config = {
    SERIAL_DEFAULT_BITRATE, (SAM3X_SERIAL_PARITY_NO) | (SAM3X_SERIAL_STOP_1) | (SAM3X_SERIAL_CHAR_8_BIT)
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/


////////USART FUNCTIONS

/* The CD value scope programmed in MR register. */
#define MIN_CD_VALUE                  0x01
#define MAX_CD_VALUE                  US_BRGR_CD_Msk

/* The receiver sampling divide of baudrate clock. */
#define HIGH_FRQ_SAMPLE_DIV           16
#define LOW_FRQ_SAMPLE_DIV            8

/**
 * \brief Calculate a clock divider(CD) and a fractional part (FP) for the
 * USART asynchronous modes to generate a baudrate as close as possible to
 * the baudrate set point.
 *
 * \note Baud rate calculation: Baudrate = ul_mck/(Over * (CD + FP/8))
 * (Over being 16 or 8). The maximal oversampling is selected if it allows to
 * generate a baudrate close to the set point.
 *
 * \param p_usart Pointer to a USART instance.
 * \param baudrate Baud rate set point.
 * \param ul_mck USART module input clock frequency.
 *
 * \retval 0 Baud rate is successfully initialized.
 * \retval 1 Baud rate set point is out of range for the given input clock
 * frequency.
 */
uint32_t usart_set_async_baudrate(usart_t u, uint32_t baudrate) {
    uint32_t over;
    uint32_t cd_fp;
    uint32_t cd;
    uint32_t fp;

    /* Calculate the receiver sampling divide of baudrate clock. */
    if (SystemCoreClock >= HIGH_FRQ_SAMPLE_DIV * baudrate) {
        over = HIGH_FRQ_SAMPLE_DIV;
    } else {
        over = LOW_FRQ_SAMPLE_DIV;
    }

    /* Calculate clock divider according to the fraction calculated formula. */
    cd_fp = (8 * SystemCoreClock + (over * baudrate) / 2) / (over * baudrate);
    cd = cd_fp >> 3;
    fp = cd_fp & 0x07;
    if (cd < MIN_CD_VALUE || cd > MAX_CD_VALUE) {
        return 1;
    }

    /* Configure the OVER bit in MR register. */
    if (over == 8) {
        u->US_MR |= US_MR_OVER;
    }

    /* Configure the baudrate generate register. */
    u->US_BRGR = (cd << US_BRGR_CD_Pos) | (fp << US_BRGR_FP_Pos);

    return 0;
}

#define usart_disable_writeprotect(u) (u)->US_WPMR = US_WPMR_WPKEY(0x555341U)
#define SERIAL_ERR_INTERRUPTS (US_IER_RXBRK | US_IER_FRAME | US_IER_PARE | US_IER_OVRE)
#define SERIAL_INTERRUPTS (US_IER_RXRDY | SERIAL_ERR_INTERRUPTS)
#define SERIAL_WR_INTERRUPTS (US_IER_TXRDY | US_IER_TXEMPTY)




void usart_reset(usart_t u) {

    /* Reset registers that could cause unpredictable behavior after reset. */
    u->US_MR = 0;
    u->US_RTOR = 0;
    u->US_TTGR = 0;

    usart_disable_writeprotect(u);
    /* Disable TX and RX. */
    usart_reset_tx(u);
    usart_reset_rx(u);
    /* Reset status bits. */
    usart_reset_status(u);
    /* Turn off RTS and DTR if exist. */
    /* not implemented */
    //usart_drive_RTS_pin_high(u);
}



uint32_t usart_init_rs232(usart_t u, const SerialConfig *cfg) {
    static uint32_t ul_reg_val;

    /* Reset the USART and shut down TX and RX. */
    usart_reset(u);

    ul_reg_val = 0;
    /* Check whether the input values are legal. */
    if (usart_set_async_baudrate(u, cfg->speed)) {
        return 1;
    }

    /* Configure the USART option. */

    /*
    ul_reg_val |= cfg->char_length | cfg->parity_type |
                  p_usart_opt->channel_mode | p_usart_opt->stop_bits;
    */
    ul_reg_val = cfg->flags | US_MR_CHMODE_NORMAL;

    /* Configure the USART mode as normal mode. */
    ul_reg_val |= US_MR_USART_MODE_NORMAL;

    u->US_MR |= ul_reg_val;

    usart_enable_tx(u);
    usart_enable_rx(u);
    usart_enable_interrupt(u, SERIAL_INTERRUPTS);

    return 0;

}


////////UART FUNCTIONS

/* UART internal div factor for sampling */
#define UART_MCK_DIV             16
/* Div factor to get the maximum baud rate */
#define UART_MCK_DIV_MIN_FACTOR  1
/* Div factor to get the minimum baud rate */
#define UART_MCK_DIV_MAX_FACTOR  65535

#define uart_reset(u) u->UART_CR = UART_CR_RSTRX | UART_CR_RSTTX | UART_CR_RXDIS | UART_CR_TXDIS
#define uart_get_status(u) u->UART_SR
#define uart_get_interrupt_mask(u) u->UART_IMR
#define uart_reset_status(u) u->UART_CR = UART_CR_RSTSTA
#define uart_rxchar(u) u->UART_RHR
#define uart_write(u,c) u->UART_THR = c
#define uart_disable_interrupt(u,intr) u->UART_IDR = intr
#define uart_enable_interrupt(u,intr) u->UART_IER = intr


#define UART_ERR_INTERRUPTS (UART_IER_FRAME | UART_IER_PARE | UART_IER_OVRE)
#define UART_INTERRUPTS (UART_IER_RXRDY | UART_ERR_INTERRUPTS)
#define UART_WR_INTERRUPTS (UART_IER_TXRDY | UART_IER_TXEMPTY)

uint32_t uart_init(Uart *p_uart, const SerialConfig *cfg) {
    uint32_t cd = 0;

    /* Reset and disable receiver & transmitter */
    p_uart->UART_CR = UART_CR_RSTRX | UART_CR_RSTTX
                      | UART_CR_RXDIS | UART_CR_TXDIS;

    /* Check and configure baudrate */
    /* Asynchronous, no oversampling */
    cd = (_system_frequency / cfg->speed) / UART_MCK_DIV;
    if (cd < UART_MCK_DIV_MIN_FACTOR || cd > UART_MCK_DIV_MAX_FACTOR)
        return 1;

    p_uart->UART_BRGR = cd;
    /* Configure mode */
    p_uart->UART_MR = cfg->flags | UART_MR_CHMODE_NORMAL;

    /* Disable PDC channel */
    p_uart->UART_PTCR = UART_PTCR_RXTDIS | UART_PTCR_TXTDIS;

    /* Enable receiver and transmitter */
    p_uart->UART_CR = UART_CR_RXEN | UART_CR_TXEN;
    p_uart->UART_IER = UART_IER_OVRE | UART_IER_FRAME | UART_IER_PARE;
    uart_disable_interrupt(p_uart, UART_WR_INTERRUPTS);
    uart_enable_interrupt(p_uart,UART_INTERRUPTS);
    return 0;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if SAM3X_SERIAL_USE_SD1 || defined(__DOXYGEN__)
static void notify1(GenericQueue *qp) {

    (void)qp;
    usart_enable_interrupt(USART0, SERIAL_WR_INTERRUPTS);

}
#endif

#if SAM3X_SERIAL_USE_SD2 || defined(__DOXYGEN__)
static void notify2(GenericQueue *qp) {

    (void)qp;
    usart_enable_interrupt(USART1, SERIAL_WR_INTERRUPTS);
}
#endif

#if SAM3X_SERIAL_USE_SD3 || defined(__DOXYGEN__)
static void notify3(GenericQueue *qp) {

    (void)qp;
    usart_enable_interrupt(USART2, SERIAL_WR_INTERRUPTS);
}
#endif

#if SAM3X_SERIAL_USE_SD4 || defined(__DOXYGEN__)
static void notify4(GenericQueue *qp) {

    (void)qp;
    usart_enable_interrupt(USART3, SERIAL_WR_INTERRUPTS);
}
#endif


#if SAM3X_SERIAL_USE_SD5 || defined(__DOXYGEN__)
static void notify5(GenericQueue *qp) {

    (void)qp;
    uart_enable_interrupt(UART, UART_WR_INTERRUPTS);
}
#endif

/**
 * @brief   Common IRQ handler.
 *
 * @param[in] sdp       communication channel associated to the USART
 */

static void serve_interrupt(SerialDriver *sdp) {
    usart_t u = sdp->u;
    uint32_t status = usart_get_status(u);
    status &= usart_get_interrupt_mask(u);

    chSysLockFromIsr();


    if (status & SERIAL_ERR_INTERRUPTS) {
        if (status & US_IER_RXBRK) {
            chnAddFlagsI(sdp, SD_BREAK_DETECTED);
        }
        if (status & US_IER_FRAME) {
            chnAddFlagsI(sdp, SD_FRAMING_ERROR);
        }
        if (status & US_IER_PARE) {
            chnAddFlagsI(sdp, SD_PARITY_ERROR);
        }
        if (status & US_IER_OVRE) {
            chnAddFlagsI(sdp, SD_OVERRUN_ERROR);
        }
        usart_reset_status(u);
    }


    if (status & US_IER_RXRDY) {
        sdIncomingDataI(sdp, usart_rxchar(u));
    }

    if (status & US_IER_TXRDY) {
        msg_t b;
        b = chOQGetI(&sdp->oqueue);
        if (b < Q_OK) {
            chnAddFlagsI(sdp, CHN_OUTPUT_EMPTY);
        } else {
            usart_write(u, b);
        }
    }

    if (status & US_IER_TXEMPTY) {
        if (chOQIsEmptyI(&sdp->oqueue)) {
            chnAddFlagsI(sdp, CHN_TRANSMISSION_END);
            usart_disable_interrupt(u, SERIAL_WR_INTERRUPTS);
        }
    }
    chSysUnlockFromIsr();
}


#if SAM3X_SERIAL_USE_SD5 || defined(__DOXYGEN__)
static void serve_uart_interrupt(SerialDriver *sdp) {
    uart_t u = UART;
    uint32_t status = uart_get_status(u);
    status &= uart_get_interrupt_mask(u);

    chSysLockFromIsr();


    if (status & UART_ERR_INTERRUPTS) {
        if (status & UART_IER_FRAME) {
            chnAddFlagsI(sdp, SD_FRAMING_ERROR);
        }
        if (status & UART_IER_PARE) {
            chnAddFlagsI(sdp, SD_PARITY_ERROR);
        }
        if (status & UART_IER_OVRE) {
            chnAddFlagsI(sdp, SD_OVERRUN_ERROR);
        }
        uart_reset_status(u);
    }


    if (status & US_IER_RXRDY) {
        sdIncomingDataI(sdp, uart_rxchar(u));
    }

    if (status & US_IER_TXRDY) {
        msg_t b;
        b = chOQGetI(&sdp->oqueue);
        if (b < Q_OK) {
            chnAddFlagsI(sdp, CHN_OUTPUT_EMPTY);
        } else {
            uart_write(u, b);
        }
    }

    if (status & US_IER_TXEMPTY) {
        if (chOQIsEmptyI(&sdp->oqueue)) {
            chnAddFlagsI(sdp, CHN_TRANSMISSION_END);
            uart_disable_interrupt(u, UART_WR_INTERRUPTS);
        }
    }
    chSysUnlockFromIsr();
}
#endif


#if SAM3X_SERIAL_USE_SD1

CH_IRQ_HANDLER(USART0_Handler) {

    CH_IRQ_PROLOGUE();

    serve_interrupt(&SD1);

    CH_IRQ_EPILOGUE();
}
#endif

#if SAM3X_SERIAL_USE_SD2
CH_IRQ_HANDLER(USART1_Handler) {

    CH_IRQ_PROLOGUE();

    serve_interrupt(&SD2);

    CH_IRQ_EPILOGUE();
}
#endif

#if SAM3X_SERIAL_USE_SD3
CH_IRQ_HANDLER(USART2_Handler) {

    CH_IRQ_PROLOGUE();

    serve_interrupt(&SD3);

    CH_IRQ_EPILOGUE();
}
#endif

#if SAM3X_SERIAL_USE_SD4
CH_IRQ_HANDLER(USART3_Handler) {

    CH_IRQ_PROLOGUE();

    serve_interrupt(&SD4);

    CH_IRQ_EPILOGUE();
}
#endif




#if SAM3X_SERIAL_USE_SD5

CH_IRQ_HANDLER(UART_Handler) {

    CH_IRQ_PROLOGUE();

    serve_uart_interrupt(&SD5);

    CH_IRQ_EPILOGUE();
}
#endif
/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/




/**
 * @brief   Low level serial driver initialization.
 *
 * @notapi
 */
void sd_lld_init(void) {

#if SAM3X_SERIAL_USE_SD1
    /* Driver initialization.*/
    sdObjectInit(&SD1, NULL, notify1);
    pmc_enable_periph_clk(ID_USART0);
    SD1.u = USART0;
#endif
#if SAM3X_SERIAL_USE_SD2
    /* Driver initialization.*/
    sdObjectInit(&SD2, NULL, notify2);
    pmc_enable_periph_clk(ID_USART1);
    SD2.u = USART1;
#endif
#if SAM3X_SERIAL_USE_SD3
    /* Driver initialization.*/
    sdObjectInit(&SD3, NULL, notify3);
    pmc_enable_periph_clk(ID_USART2);
    SD3.u = USART2;
#endif
#if SAM3X_SERIAL_USE_SD4
    /* Driver initialization.*/
    sdObjectInit(&SD4, NULL, notify4);
    pmc_enable_periph_clk(ID_USART3);
    SD4.u = USART3;
#endif
#if SAM3X_SERIAL_USE_SD5
    /* Driver initialization.*/
    sdObjectInit(&SD5, NULL, notify5);
    pmc_enable_periph_clk(ID_UART);
    SD5.u = NULL;//only one uart
#endif

}


/**
 * @brief   Low level serial driver configuration and (re)start.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] config    the architecture-dependent serial driver configuration.
 *                      If this parameter is set to @p NULL then a default
 *                      configuration is used.
 *
 * @notapi
 */
void sd_lld_start(SerialDriver *sdp, const SerialConfig *config) {

    if (config == NULL)
        config = &default_config;

    if (sdp->state == SD_STOP) {
        /* Enables the peripheral.*/
#if SAM3X_SERIAL_USE_SD1
        if (&SD1 == sdp) {
            usart_init_rs232(sdp->u, config);
            nvicEnableVector(USART0_IRQn, CORTEX_PRIORITY_MASK(SAM3X_USART0_IRQ_PRIORITY));
        }
#endif

#if SAM3X_SERIAL_USE_SD2
        if (&SD2 == sdp) {
            /* set tx */
            //palSetPadMode(IOPORTA, 13, PAD_TO_A | PAL_MODE_OUTPUT_PUSHPULL);
            /* set rx */
            //palSetPadMode(IOPORTA, 12, PAD_TO_A);
            usart_init_rs232(sdp->u, config);
            nvicEnableVector(USART1_IRQn, CORTEX_PRIORITY_MASK(SAM3X_USART1_IRQ_PRIORITY));
        }
#endif

#if SAM3X_SERIAL_USE_SD3
        if (&SD3 == sdp) {
            usart_init_rs232(sdp->u, config);
            nvicEnableVector(USART2_IRQn, CORTEX_PRIORITY_MASK(SAM3X_USART2_IRQ_PRIORITY));
        }
#endif

#if SAM3X_SERIAL_USE_SD4
        if (&SD4 == sdp) {
            usart_init_rs232(sdp->u, config);
            nvicEnableVector(USART3_IRQn, CORTEX_PRIORITY_MASK(SAM3X_USART3_IRQ_PRIORITY));
        }
#endif


#if SAM3X_SERIAL_USE_SD5
        if (&SD5 == sdp) {
            uart_init(UART, config);
            //blink(0x0900, 2); //led0
            nvicEnableVector(UART_IRQn, CORTEX_PRIORITY_MASK(SAM3X_USART4_IRQ_PRIORITY));
            //blink(0x0900, 2); //led0
        }
#endif


    }
    /* Configures the peripheral.*/

}

/**
 * @brief   Low level serial driver stop.
 * @details De-initializes the USART, stops the associated clock, resets the
 *          interrupt vector.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 *
 * @notapi
 */
void sd_lld_stop(SerialDriver *sdp) {

    if (sdp->state == SD_READY) {
        /* Resets the peripheral.*/

        /* Disables the peripheral.*/
#if SAM3X_SERIAL_USE_SD5
        if (sdp != &SD5)
#endif
        {
            usart_reset(sdp->u);
            usart_disable_interrupt(sdp->u, SERIAL_INTERRUPTS);
        }
#if SAM3X_SERIAL_USE_SD1
        if (&SD1 == sdp) {
            nvicDisableVector(USART0_IRQn);
        }
#endif
#if SAM3X_SERIAL_USE_SD2
        if (&SD2 == sdp) {
            nvicDisableVector(USART1_IRQn);
        }
#endif
#if SAM3X_SERIAL_USE_SD3
        if (&SD3 == sdp) {
            nvicDisableVector(USART2_IRQn);
        }
#endif
#if SAM3X_SERIAL_USE_SD4
        if (&SD4 == sdp) {
            nvicDisableVector(USART3_IRQn);
        }
#endif
#if SAM3X_SERIAL_USE_SD5
        if (&SD5 == sdp) {
            uart_reset(UART);
            uart_disable_interrupt(UART, UART_INTERRUPTS);
            nvicDisableVector(UART_IRQn);
        }
#endif
    }
}

#endif /* HAL_USE_SERIAL */

/** @} */

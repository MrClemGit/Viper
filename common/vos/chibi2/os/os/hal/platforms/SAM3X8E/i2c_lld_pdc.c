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
 * @file    templates/i2c_lld.c
 * @brief   I2C Driver subsystem low level driver source template.
 *
 * @addtogroup I2C
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_I2C || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   I2C1 driver identifier.
 */
#if SAM3X_I2C_USE_I2C1 || defined(__DOXYGEN__)
I2CDriver I2CD1;
#endif

#if SAM3X_I2C_USE_I2C2 || defined(__DOXYGEN__)
I2CDriver I2CD2;
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


#define wakeup_isr(i2cp, msg) {                                             \
        chSysLockFromIsr();                                                       \
        if ((i2cp)->thread != NULL) {                                             \
            Thread *tp = (i2cp)->thread;                                            \
            (i2cp)->thread = NULL;                                                  \
            tp->p_u.rdymsg = (msg);                                                 \
            chSchReadyI(tp);                                                        \
        }                                                                         \
        chSysUnlockFromIsr();                                                     \
    }


void serve_i2c_interrupt(I2CDriver *i2cp) {
    i2cdef_t i2c = i2cp->i2c;
    uint32_t status = i2c->TWI_SR;
    msg_t msg;

    if (status & TWI_SR_ENDRX) {
        palTogglePad(IOPORTA, 21);

        /* Disable PDC */
        i2c->TWI_PTCR = TWI_PTCR_RXTDIS;
        i2c->TWI_IDR = TWI_IDR_ENDRX;


        /* Wait for RX ready flag */
        while (1) {
            status = i2c->TWI_SR;
            if (status & TWI_SR_RXRDY) {
                break;
            }
        }

        /* Complete the transfer. */
        i2c->TWI_CR = TWI_CR_STOP;
        /* Read second last data */
        *i2cp->curbuf++ = i2c->TWI_RHR;

        /* Wait for RX ready flag */
        while (1) {
            status = i2c->TWI_SR;
            if (status & TWI_SR_RXRDY) {
                break;
            }
        }

        /* Read last data */
        *i2cp->curbuf++ = i2c->TWI_RHR;
        /* Wait for TX complete flag before releasing semaphore */
        while (1) {
            status = i2c->TWI_SR;
            if (status & TWI_SR_TXCOMP) {
                break;
            }
        }

        msg = RDY_OK;
    } else if (status & (TWI_SR_NACK | TWI_SR_OVRE)) {
        msg = RDY_RESET;
        i2cp->errors = status;
    }
    wakeup_isr(i2cp, msg);
}


#if SAM3X_I2C_USE_I2C1
CH_IRQ_HANDLER(TWI0_Handler) {
    CH_IRQ_PROLOGUE();
    serve_i2c_interrupt(&I2CD1);
    CH_IRQ_EPILOGUE();
}
#endif


#if SAM3X_I2C_USE_I2C2
CH_IRQ_HANDLER(TWI1_Handler) {
    CH_IRQ_PROLOGUE();
    serve_i2c_interrupt(&I2CD2);
    CH_IRQ_EPILOGUE();
}
#endif


/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level I2C driver initialization.
 *
 * @notapi
 */
void i2c_lld_init(void) {

#if SAM3X_I2C_USE_I2C1
    i2cObjectInit(&I2CD1);
    pmc_enable_periph_clk(ID_TWI0);
    I2CD1.i2c = TWI0;
    nvicEnableVector(TWI0_IRQn, CORTEX_PRIORITY_MASK(SAM3X_I2C_IRQ_PRIORITY));
#endif

#if SAM3X_I2C_USE_I2C2
    i2cObjectInit(&I2CD2);
    pmc_enable_periph_clk(ID_TWI1);
    I2CD2.i2c = TWI1;
    nvicEnableVector(TWI1_IRQn, CORTEX_PRIORITY_MASK(SAM3X_I2C_IRQ_PRIORITY));
#endif

}


#define I2C_FAST_MODE_SPEED  400000
#define TWI_CLK_DIVIDER      2
#define TWI_CLK_CALC_ARGU    4
#define TWI_CLK_DIV_MAX      0xFF
#define TWI_CLK_DIV_MIN      7

void i2c_master_init(I2CDriver *i2cp) {
    i2cdef_t i2c = i2cp->i2c;
    uint32_t ckdiv = 0;
    uint32_t c_lh_div;

    /* Disable TWI interrupts */
    i2c->TWI_IDR = ~0UL;

    /* Dummy read in status register */
    i2c->TWI_SR;

    /* Reset TWI peripheral */
    i2c->TWI_CR = TWI_CR_SWRST;
    i2c->TWI_RHR;


    /* enable master mode */
    i2c->TWI_CR = TWI_CR_MSDIS;
    i2c->TWI_CR = TWI_CR_SVDIS;
    i2c->TWI_CR = TWI_CR_MSEN;

    /* Select the speed */
    c_lh_div = SystemCoreClock / (i2cp->config->bitrate * TWI_CLK_DIVIDER) - TWI_CLK_CALC_ARGU;

    /* cldiv must fit in 8 bits, ckdiv must fit in 3 bits */
    while ((c_lh_div > TWI_CLK_DIV_MAX) && (ckdiv < TWI_CLK_DIV_MIN)) {
        /* Increase clock divider */
        ckdiv++;
        /* Divide cldiv value */
        c_lh_div /= TWI_CLK_DIVIDER;
    }

    /* set clock waveform generator register */
    i2c->TWI_CWGR = TWI_CWGR_CLDIV(c_lh_div) | TWI_CWGR_CHDIV(c_lh_div) | TWI_CWGR_CKDIV(ckdiv);

}


/**
 * @brief   Configures and activates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
void i2c_lld_start(I2CDriver *i2cp) {

    if (i2cp->state == I2C_STOP) {
        /* Enables the peripheral.*/
#if SAM3X_I2C_USE_I2C1
        if (&I2CD1 == i2cp) {
            palSetPadMode(IOPORTA, 18, PAD_TO_A);
            palSetPadMode(IOPORTA, 17, PAD_TO_A);
        }
#endif
#if SAM3X_I2C_USE_I2C2
        if (&I2CD2 == i2cp) {
            palSetPadMode(IOPORTB, 13, PAD_TO_A);
            palSetPadMode(IOPORTB, 12, PAD_TO_A);
        }
#endif
        i2c_master_init(i2cp);
    }
    /* Configures the peripheral.*/

}

/**
 * @brief   Deactivates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
void i2c_lld_stop(I2CDriver *i2cp) {

    if (i2cp->state != I2C_STOP) {
        /* Resets the peripheral.*/

        /* Disables the peripheral.*/
#if SAM3X_I2C_USE_I2C1
        if (&I2CD1 == i2cp) {

        }
#endif
#if SAM3X_I2C_USE_I2C2
        if (&I2CD2 == i2cp) {

        }
#endif
    }
}

static uint32_t twi_mk_addr(const uint8_t *addr, int len) {
    uint32_t val;

    if (len == 0)
        return 0;

    val = addr[0];
    if (len > 1) {
        val <<= 8;
        val |= addr[1];
    }
    if (len > 2) {
        val <<= 8;
        val |= addr[2];
    }
    return val;
}

/**
 * @brief   Receives data via the I2C bus as master.
 * @details Number of receiving bytes must be more than 1 on STM32F1x. This is
 *          hardware restriction.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] addr      slave device address
 * @param[out] rxbuf    pointer to the receive buffer
 * @param[in] rxbytes   number of bytes to be received
 * @param[in] timeout   the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The operation status.
 * @retval RDY_OK       if the function succeeded.
 * @retval RDY_RESET    if one or more I2C errors occurred, the errors can
 *                      be retrieved using @p i2cGetErrors().
 * @retval RDY_TIMEOUT  if a timeout occurred before operation end. <b>After a
 *                      timeout the driver must be stopped and restarted
 *                      because the bus is in an uncertain state</b>.
 *
 * @notapi
 */
msg_t i2c_lld_master_receive_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                     uint8_t *rxbuf, size_t rxbytes,
                                     systime_t timeout) {

    (void)i2cp;
    (void)addr;
    (void)rxbuf;
    (void)rxbytes;
    (void)timeout;
    uint32_t status;
    uint32_t stop_sent;
    i2cdef_t i2c = i2cp->i2c;

    /* Set read mode, slave address and 3 internal address byte lengths */
    i2c->TWI_MMR = 0;
    i2c->TWI_MMR = TWI_MMR_MREAD | TWI_MMR_DADR(addr->chip) | ((addr->len << TWI_MMR_IADRSZ_Pos) & TWI_MMR_IADRSZ_Msk);

    /* Set internal address for remote chip */
    i2c->TWI_IADR = 0;
    i2c->TWI_IADR = twi_mk_addr(addr->addr, addr->len);




    /* Send a START condition */
    if (rxbytes = 1) {
        i2c->TWI_CR = TWI_CR_START | TWI_CR_STOP;
        while (!(i2c->TWI_SR & TWI_SR_RXRDY));
        *rxbuf = i2c->TWI_RHR;

    } else {
        i2c->TWI_PTCR = TWI_PTCR_RXTDIS | TWI_PTCR_TXTDIS;


        i2c->TWI_IER = TWI_IER_ENDRX | TWI_IER_NACK | TWI_IER_OVRE;

        chSysLock();
        i2cp->thread = chThdSelf();
        i2c->TWI_RNPR = 0;
        i2c->TWI_RNCR = 0;
        i2c->TWI_RPR = (uint32_t) rxbuf;
        i2c->TWI_RCR = rxbytes - 2;
        i2c->TWI_PTCR = TWI_PTCR_RXTEN;
        i2cp->curbuf = rxbuf + (rxbytes - 2);

        i2c->TWI_CR = TWI_CR_START;
        chSchGoSleepS(THD_STATE_SUSPENDED);

        return chThdSelf()->p_u.rdymsg;
    }

    return RDY_OK;
}

/**
 * @brief   Transmits data via the I2C bus as master.
 * @details Number of receiving bytes must be 0 or more than 1 on STM32F1x.
 *          This is hardware restriction.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] addr      slave device address
 * @param[in] txbuf     pointer to the transmit buffer
 * @param[in] txbytes   number of bytes to be transmitted
 * @param[out] rxbuf    pointer to the receive buffer
 * @param[in] rxbytes   number of bytes to be received
 * @param[in] timeout   the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The operation status.
 * @retval RDY_OK       if the function succeeded.
 * @retval RDY_RESET    if one or more I2C errors occurred, the errors can
 *                      be retrieved using @p i2cGetErrors().
 * @retval RDY_TIMEOUT  if a timeout occurred before operation end. <b>After a
 *                      timeout the driver must be stopped and restarted
 *                      because the bus is in an uncertain state</b>.
 *
 * @notapi
 */
msg_t i2c_lld_master_transmit_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                      const uint8_t *txbuf, size_t txbytes,
                                      uint8_t *rxbuf, size_t rxbytes,
                                      systime_t timeout) {

    (void)i2cp;
    (void)addr;
    (void)txbuf;
    (void)txbytes;
    (void)rxbuf;
    (void)rxbytes;
    (void)timeout;

    i2cdef_t i2c = i2cp->i2c;
    uint32_t status;

    /* Set write mode, slave address and 3 internal address byte lengths */
    i2c->TWI_MMR = 0;
    i2c->TWI_MMR = TWI_MMR_DADR(addr->chip) |
                   ((addr->len << TWI_MMR_IADRSZ_Pos) &
                    TWI_MMR_IADRSZ_Msk);

    /* Set internal address for remote chip */
    i2c->TWI_IADR = 0;
    i2c->TWI_IADR = twi_mk_addr(addr->addr, addr->len);

    /* Send all bytes */
    while (txbytes > 0) {
        status = i2c->TWI_SR;
        if (status & TWI_SR_NACK) {
            return RDY_RESET;
        }

        if (!(status & TWI_SR_TXRDY)) {
            continue;
        }
        i2c->TWI_THR = *txbuf++;

        txbytes--;
    }

    while (1) {
        status = i2c->TWI_SR;
        if (status & TWI_SR_NACK) {
            return RDY_RESET;
        }

        if (status & TWI_SR_TXRDY) {
            break;
        }
    }

    i2c->TWI_CR = TWI_CR_STOP;

    while (!(i2c->TWI_SR & TWI_SR_TXCOMP)) {
    }


    return RDY_OK;
}

#endif /* HAL_USE_I2C */

/** @} */

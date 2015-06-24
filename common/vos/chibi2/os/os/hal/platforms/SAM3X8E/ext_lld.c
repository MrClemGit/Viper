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
 * @file    templates/ext_lld.c
 * @brief   EXT Driver subsystem low level driver source template.
 *
 * @addtogroup EXT
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_EXT || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   EXT1 driver identifier.
 */
EXTDriver EXTD1;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

static const extioport_t extports[] = {PIOA, PIOB, PIOC, PIOD};


/* PIO_ISR is cleared right after reading it */
#define EXT_INTERRUPT_STATUS(port) (port)->PIO_ISR;
#define EXT_INTERRUPT_MASK(port) (port)->PIO_IMR;
#define EXT_ENABLE_INTERRUPT(port,mask) EXT_INTERRUPT_STATUS(port); (port)->PIO_IER = (mask)
#define EXT_DISABLE_INTERRUPT(port,mask) (port)->PIO_IDR = (mask)


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
 * @brief   Low level EXT driver initialization.
 *
 * @notapi
 */
void ext_lld_init(void) {
    /* Driver initialization.*/
    extObjectInit(&EXTD1);
}

/**
 * @brief   Configures and activates the EXT peripheral.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 *
 * @notapi
 */
void ext_lld_start(EXTDriver *extp) {

    if (extp->state == EXT_STOP) {
        EXT_DISABLE_INTERRUPT(extports[EXT_PORTA], 0xffffffff);
        EXT_DISABLE_INTERRUPT(extports[EXT_PORTB], 0xffffffff);
        EXT_DISABLE_INTERRUPT(extports[EXT_PORTC], 0xffffffff);
        EXT_DISABLE_INTERRUPT(extports[EXT_PORTD], 0xffffffff);
        /* Enables the peripheral.*/
        nvicEnableVector(PIOA_IRQn, CORTEX_PRIORITY_MASK(SAM3X_PIOA_IRQ_PRIORITY));
        nvicEnableVector(PIOB_IRQn, CORTEX_PRIORITY_MASK(SAM3X_PIOB_IRQ_PRIORITY));
        nvicEnableVector(PIOC_IRQn, CORTEX_PRIORITY_MASK(SAM3X_PIOC_IRQ_PRIORITY));
        nvicEnableVector(PIOD_IRQn, CORTEX_PRIORITY_MASK(SAM3X_PIOD_IRQ_PRIORITY));
    }
    /* Configures the peripheral.*/

}

/**
 * @brief   Deactivates the EXT peripheral.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 *
 * @notapi
 */
void ext_lld_stop(EXTDriver *extp) {

    if (extp->state == EXT_ACTIVE) {
        /* Resets the peripheral.*/
        nvicDisableVector(PIOA_IRQn);
        nvicDisableVector(PIOB_IRQn);
        nvicDisableVector(PIOC_IRQn);
        nvicDisableVector(PIOD_IRQn);
    }
}




/**
 * @brief   Enables an EXT channel.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 * @param[in] channel   channel to be enabled
 *
 * @notapi
 */


void ext_lld_channel_enable(EXTDriver *extp, expchannel_t channel) {

    uint32_t pmask;
    extioport_t port;
    EXTChannelConfig *cfg;
    (void)extp;
    (void)channel;

    chSysLock();

    cfg = &extp->config->channels[channel];
    cfg->enabled = 1;

    port = extports[cfg->ioport];
    pmask = (1u << (cfg->pad));

    if (cfg->mode == EXT_CH_MODE_BOTH_EDGES) {
        port->PIO_AIMDR = pmask;
    } else {
        /* activate special modes */
        port->PIO_AIMER = pmask;
        if (cfg->mode == EXT_CH_MODE_RISING_EDGE) {
            port->PIO_REHLSR = pmask;
        } else {
            port->PIO_FELLSR = pmask;
        }
        /* activate edge selection */
        port->PIO_ESR = pmask;
    }

    EXT_ENABLE_INTERRUPT(port, pmask);
    chSysUnlock();
}

/**
 * @brief   Disables an EXT channel.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 * @param[in] channel   channel to be disabled
 *
 * @notapi
 */
void ext_lld_channel_disable(EXTDriver *extp, expchannel_t channel) {

    EXTChannelConfig *cfg;

    chSysLock();
    cfg = &extp->config->channels[channel];

    cfg->enabled = 0;
    EXT_DISABLE_INTERRUPT(extports[cfg->ioport], (1u << cfg->pad));
    chSysUnlock()
}



/*
  Low Level functions
*/


void ext_lld_process_irq(extioport_t port, uint32_t portid) {
    uint32_t status;
    uint32_t i = 0;
    uint32_t pmask;
    EXTChannelConfig *cfg = EXTD1.config->channels;

    chSysLockFromIsr();
    /* Read PIO controller status */
    status = EXT_INTERRUPT_STATUS(port);
    status &= EXT_INTERRUPT_MASK(port);

    while (status != 0) {
        /* Source is configured on the same controller */
        pmask = (1u << cfg[i].pad);
        if (cfg[i].enabled &&
                cfg[i].ioport == portid &&
                (status & pmask) ) {
            /* enabled & same port & status bit on => callback!*/
            cfg[i].cb(&EXTD1, i, port->PIO_PDSR&pmask);
            /* interrupt handled */
            status &= ~(pmask);
        }
        i++;
    }
}



CH_IRQ_HANDLER(PIOA_Handler) {

    CH_IRQ_PROLOGUE();
    ext_lld_process_irq(PIOA, EXT_PORTA);
    CH_IRQ_EPILOGUE();
}

CH_IRQ_HANDLER(PIOB_Handler) {

    CH_IRQ_PROLOGUE();
    ext_lld_process_irq(PIOB, EXT_PORTB);
    CH_IRQ_EPILOGUE();
}


CH_IRQ_HANDLER(PIOC_Handler) {

    CH_IRQ_PROLOGUE();
    ext_lld_process_irq(PIOC, EXT_PORTC);
    CH_IRQ_EPILOGUE();
}

CH_IRQ_HANDLER(PIOD_Handler) {

    CH_IRQ_PROLOGUE();
    ext_lld_process_irq(PIOD, EXT_PORTD);
    CH_IRQ_EPILOGUE();
}


#endif /* HAL_USE_EXT */

/** @} */

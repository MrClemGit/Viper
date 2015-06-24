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
 * @file    templates/pwm_lld.c
 * @brief   PWM Driver subsystem low level driver source template.
 *
 * @addtogroup PWM
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_PWM || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   PWM1 driver identifier.
 */
#if SAM3X_PWM_USE_PWM || defined(__DOXYGEN__)
PWMDriver PWMD1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

static Pwm *p_pwm = PWM;
#define PWM_INVALID_ARGUMENT  0xFFFF
#define PWM_CLOCK_DIV_MAX  256
#define PWM_CLOCK_PRE_MAX  11


/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * \brief Find a prescaler/divisor couple to generate the desired ul_frequency
 * from ul_mck.
 *
 * \param ul_frequency Desired frequency in Hz.
 * \param ul_mck Master clock frequency in Hz.
 *
 * \retval Return the value to be set in the PWM Clock Register (PWM Mode Register for
 * SAM3N/SAM4N/SAM4C/SAM4CP/SAM4CM) or PWM_INVALID_ARGUMENT if the configuration cannot be met.
 */
static uint32_t pwm_clocks_generate(uint32_t ul_frequency) {
    uint32_t ul_divisors[PWM_CLOCK_PRE_MAX] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024 };
    uint32_t ul_pre = 0;
    uint32_t ul_div;
    uint32_t ul_mck = SystemCoreClock;

    /* Find prescaler and divisor values */
    do {
        ul_div = (ul_mck / ul_divisors[ul_pre]) / ul_frequency;
        if (ul_div <= PWM_CLOCK_DIV_MAX) {
            break;
        }
        ul_pre++;
    } while (ul_pre < PWM_CLOCK_PRE_MAX);

    /* Return result */
    if (ul_pre < PWM_CLOCK_PRE_MAX) {
        return ul_div | (ul_pre << 8);
    } else {
        return PWM_INVALID_ARGUMENT;
    }
}

/**
 * \brief Initialize the PWM source clock (clock A and clock B).
 *
 * \param p_pwm Pointer to a PWM instance.
 * \param clock_config PWM clock configuration.
 *
 * \retval 0 if initialization succeeds, otherwise fails.
 */
uint32_t pwm_init(uint32_t ul_clka) {
    uint32_t clock = 0;
    uint32_t result;

    /* Clock A */
    if (ul_clka != 0) {
        result = pwm_clocks_generate(ul_clka);
        if (result == PWM_INVALID_ARGUMENT) {
            return result;
        }

        clock = result;
    }

    p_pwm->PWM_CLK = clock;
    return 0;
}


uint32_t pwm_channel_init(uint32_t ch_num, uint16_t ul_period, uint16_t ul_duty) {
    uint32_t tmp_reg = 0;

    /* Channel Mode/Clock Register */
    tmp_reg = (PWM_CMR_CPRE_CLKA & 0xF);
    p_pwm->PWM_CH_NUM[ch_num].PWM_CMR = tmp_reg;

    /* Channel Duty Cycle Register */
    p_pwm->PWM_CH_NUM[ch_num].PWM_CDTY = ul_duty;

    /* Channel Period Register */
    p_pwm->PWM_CH_NUM[ch_num].PWM_CPRD = ul_period;

    return 0;
}

/**
 * \brief Change the period of the PWM channel.
 *
 * \param p_pwm Pointer to a PWM instance.
 * \param p_channel Configurations of the specified PWM channel.
 * \param ul_period New period value.
 *
 * \retval 0 if changing succeeds, otherwise fails.
 */
uint32_t pwm_channel_update_period(uint32_t ch_num, uint32_t ul_period) {

    /** Check parameter */
    if (PWMD1.config->channels[ch_num].duty > ul_period) {
        return PWM_INVALID_ARGUMENT;
    } else {
        /* Save new period value */
        PWMD1.config->channels[ch_num].period = ul_period;
        p_pwm->PWM_CH_NUM[ch_num].PWM_CPRDUPD = ul_period;
    }

    return 0;
}

/**
 * \brief Change the duty cycle of the PWM channel.
 *
 * \param p_pwm Pointer to a PWM instance.
 * \param p_channel Configurations of the specified PWM channel.
 * \param ul_duty New duty cycle value.
 *
 * \retval 0 if changing succeeds, otherwise fails.
 */
uint32_t pwm_channel_update_duty(uint32_t ch_num, uint32_t ul_duty) {

    /** Check parameter */
    if (PWMD1.config->channels[ch_num].period < ul_duty) {
        return PWM_INVALID_ARGUMENT;
    } else {
        /* Save new duty cycle value */
        PWMD1.config->channels[ch_num].duty = ul_duty;
        p_pwm->PWM_CH_NUM[ch_num].PWM_CDTYUPD = ul_duty;
    }

    return 0;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if SAM3X_PWM_USE_PWM

CH_IRQ_HANDLER(PWM_Handler) {

    CH_IRQ_PROLOGUE();
    uint32_t status = p_pwm->PWM_ISR1; /*status*/
    int i;
    status &= p_pwm->PWM_IMR1;/* & interrupt_mask */

    for (i = 0; i < PWM_CHANNELS; i++) {
        if ((status & (1 << i)) && PWMD1.config->channels[i].mode && PWMD1.config->channels[i].callback) {
            //channel i duty cycle event!
            PWMD1.config->channels[i].callback(&PWMD1);
        }
    }

    //TODO: handle period interrupts @ ISR2

    CH_IRQ_EPILOGUE();
}
#endif



/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level PWM driver initialization.
 *
 * @notapi
 */
void pwm_lld_init(void) {

#if SAM3X_PWM_USE_PWM
    /* Driver initialization.*/
    pwmObjectInit(&PWMD1);
#endif
}

/**
 * @brief   Configures and activates the PWM peripheral.
 *
 * @param[in] pwmp      pointer to the @p PWMDriver object
 *
 * @notapi
 */
void pwm_lld_start(PWMDriver *pwmp) {

    if (pwmp->state == PWM_STOP) {
        /* Enables the peripheral.*/
#if SAM3X_PWM_USE_PWM
        if (&PWMD1 == pwmp) {
            int i;
            PWMConfig *cfg = pwmp->config;
            pmc_enable_periph_clk(ID_PWM);
            /* disable all channels */
            for (i = 0; i < PWM_CHANNELS; i++) {
                pwm_lld_disable_channel(pwmp, i);
            }
            pwm_init(cfg->frequency);
        }
#endif
    }
    /* Configures the peripheral.*/

}

/**
 * @brief   Deactivates the PWM peripheral.
 *
 * @param[in] pwmp      pointer to the @p PWMDriver object
 *
 * @notapi
 */
void pwm_lld_stop(PWMDriver *pwmp) {

    if (pwmp->state == PWM_READY) {
        /* Resets the peripheral.*/

        /* Disables the peripheral.*/
#if SAM3X_PWM_USE_PWM
        if (&PWMD1 == pwmp) {
            pmc_disable_periph_clk(ID_PWM);

        }
#endif
    }
}

/**
 * @brief   Changes the period the PWM peripheral.
 * @details This function changes the period of a PWM unit that has already
 *          been activated using @p pwmStart().
 * @pre     The PWM unit must have been activated using @p pwmStart().
 * @post    The PWM unit period is changed to the new value.
 * @note    The function has effect at the next cycle start.
 * @note    If a period is specified that is shorter than the pulse width
 *          programmed in one of the channels then the behavior is not
 *          guaranteed.
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 * @param[in] period    new cycle time in ticks
 *
 * @notapi
 */
void pwm_lld_change_period(PWMDriver *pwmp, pwmcnt_t period) {

    (void)pwmp;
    (void)period;

}

/**
 * @brief   Enables a PWM channel.
 * @pre     The PWM unit must have been activated using @p pwmStart().
 * @post    The channel is active using the specified configuration.
 * @note    Depending on the hardware implementation this function has
 *          effect starting on the next cycle (recommended implementation)
 *          or immediately (fallback implementation).
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 * @param[in] channel   PWM channel identifier (0...PWM_CHANNELS-1)
 * @param[in] width     PWM pulse width as clock pulses number
 *
 * @notapi
 */
void pwm_lld_enable_channel(PWMDriver *pwmp,
                            pwmchannel_t channel,
                            pwmcnt_t width) {

    (void)pwmp;
    (void)channel;
    (void)width;
    pwmp->config->channels[channel].duty = width;
    pwm_channel_init(channel, pwmp->config->channels[channel].period, pwmp->config->channels[channel].duty);
    p_pwm->PWM_ENA = (1 << channel);
    pwmp->config->channels[channel].mode = 1;
}

/**
 * @brief   Disables a PWM channel.
 * @pre     The PWM unit must have been activated using @p pwmStart().
 * @post    The channel is disabled and its output line returned to the
 *          idle state.
 * @note    Depending on the hardware implementation this function has
 *          effect starting on the next cycle (recommended implementation)
 *          or immediately (fallback implementation).
 *
 * @param[in] pwmp      pointer to a @p PWMDriver object
 * @param[in] channel   PWM channel identifier (0...PWM_CHANNELS-1)
 *
 * @notapi
 */
void pwm_lld_disable_channel(PWMDriver *pwmp, pwmchannel_t channel) {

    (void)pwmp;
    (void)channel;
    p_pwm->PWM_DIS = (1 << channel);
    pwmp->config->channels[channel].mode = 0;
    pwmp->config->channels[channel].callback = NULL;
    pwmp->config->channels[channel].duty = 0;
    pwmp->config->channels[channel].period = 0;
}

#endif /* HAL_USE_PWM */

/** @} */

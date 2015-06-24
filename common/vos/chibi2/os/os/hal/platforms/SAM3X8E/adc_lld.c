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
 * @file    templates/adc_lld.c
 * @brief   ADC Driver subsystem low level driver source template.
 *
 * @addtogroup ADC
 * @{
 */

#include "ch.h"
#include "hal.h"
#include "stdarg.h"

#if HAL_USE_ADC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   ADC1 driver identifier.
 */
#if SAM3X_ADC_USE_ADC1 || defined(__DOXYGEN__)
ADCDriver ADCD1;
#endif

#define adcd ADC


/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/
volatile uint32_t sample_pos = 0;

CH_IRQ_HANDLER(ADC_Handler) {
    uint32_t status;
    CH_IRQ_PROLOGUE();
    status = adcd->ADC_ISR;

/*
    if (status & ADC_ISR_DRDY) {
        ADCD1.samples[sample_pos++] = adcd->ADC_LCDR;
        if (sample_pos >= ADCD1.depth) {
            //end of transfer
            adc_lld_stop_conversion(&ADCD1);
            _adc_wakeup_isr(&ADCD1);
        } else {
            //adcd->ADC_IER = ADC_IER_DRDY;
            //adcd->ADC_CR = ADC_CR_START;
        }
    }
*/
    
    if (status & (ADC_ISR_RXBUFF|ADC_ISR_ENDRX)) {
        // we are at end buffer
        //second_half:
        //palSetPadMode(IOPORTC, 30, PAL_MODE_OUTPUT_PUSHPULL);
        _adc_isr_full_code(&ADCD1);
        //palSetPad(IOPORTC,30);
    }
    /* else if (status & ADC_ISR_ENDRX) {
        // half pdc transfer trigger: we can be at half buffer or end buffer
        if (ADCD1.grpp->circular) {
            //set pdc for next transfer
            if (adcd->ADC_RPR > ((uint32_t) ADCD1.samples)) {
                //we are transferring the second half: set the first half as the next transfer
                adcd->ADC_RNPR = (uint32_t) ADCD1.samples;
                adcd->ADC_RNCR = ADCD1.grpp->num_channels * (ADCD1.depth / 2);
                /// callback get the first half
                _adc_isr_half_code(&ADCD1);
            } else {
                //we are transferring the first half: set the second half as the next transfer
                adcd->ADC_RNCR = ADCD1.grpp->num_channels * (ADCD1.depth / 2);
                adcd->ADC_RNPR = (uint32_t) ADCD1.samples + adcd->ADC_RNCR;
                goto second_half;
            }
        } else {
            //this is the last half transfer
            //pdc next register ar automatically set to zero
            //adcd->ADC_RNPR = 0;
            //adcd->ADC_RNCR = 0;
            _adc_isr_half_code(&ADCD1);
        }
    }*/

    CH_IRQ_EPILOGUE();
}


/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level ADC driver initialization.
 *
 * @notapi
 */
void adc_lld_init(void) {

#if SAM3X_ADC_USE_ADC1
    /* Driver initialization.*/
    adcObjectInit(&ADCD1);
#endif /* PLATFORM_ADC_USE_ADC1 */
}






void adc_configure(ADCDriver *adcp) {
    uint32_t prescaler;
    //uint32_t adc_clock;
    uint32_t tracking = 16;
    //uint32_t error = 0;

    /* software reset */
    adcd->ADC_CR = ADC_CR_SWRST;
    /* Reset Mode Register. */
    adcd->ADC_MR = 0;

    /* Reset PDC transfer. */
    adcd->ADC_PTCR = (ADC_PTCR_RXTDIS | ADC_PTCR_TXTDIS);
    adcd->ADC_RCR = 0;
    adcd->ADC_RNCR = 0;

    /* calculating prescaler and adc parameters to obtain something near the requested samplerate: */
    /* - fix tranfer to 5, settling to 5 and tracking to 6 -> 16
       - calculate adc clock as 16*samplerate
       - obtain prescaler

       ex.    1 MHz samplerate:
                tracking      adc_clock         prescaler         error
                16            26                0                 60
                15            25                0                 68
                14            24                0                 75
                13            23                0                 82
                12            22                0                 90
                11            21                1                 0
       ex.    8 kHz
                16            208 kHz           201               91
                15            200 kHz           209               0
    */

    /*
        do {
            adc_clock = (8 + tracking) * adcp->config->samplerate;
            prescaler = (SystemCoreClock*10) / (2 * adc_clock) - 1;
            error = prescaler % 10;
            if (error > 1 && error < 9) {
                tracking--;
            } else error = 0;
        } while (error  && tracking > 0);
        prescaler /= 10;

        if (prescaler > 255) prescaler = 255;
    */
    prescaler = halGetCounterFrequency() / 40000000 - 1; //--> adcfreq @20MHz
    //adcfreq = mck/((prescaler+1)*2) ---> psc=0 adcfreq = 22Mhz (max)

    adcp->mr = ADC_MR_PRESCAL(prescaler) | (adcp->config->startup)
               | ADC_MR_TRACKTIM(tracking - 1) | ADC_MR_TRANSFER(1) |  ADC_MR_SETTLING_AST3
               | adcp->config->resolution
               //| ((adcp->config->sleepmode) ? (ADC_MR_SLEEP_SLEEP) : (ADC_MR_SLEEP_NORMAL))
               | ADC_MR_SLEEP_NORMAL
               | (adcp->config->trigger);
    adcd->ADC_MR = adcp->mr;
}


/*
 a channel can be present in a sequence only once even if the adc hardware allows more...
 TODO: remove this limitation
 */
/*
void adc_prepare_conversion(ADCConversionGroup *grpp, int n, ...) {
   int i;
   int nch = 0;
   int ch;
   int seqn = 0;
   va_list vl;
   va_start(vl, n);

   grpp->seq1 = grpp->seq2 = 0;
   grpp->channels = grpp->num_channels = 0;

   if (n < 0) n = 0;
   else if (n > 16) n = 16;

   for (i = 0; i < n; i++) {
       ch = va_arg(vl, int);
       if (!(grpp->channels & (1u << ch)))
           grpp->num_channels++;
       grpp->channels |= (1u << ch);
       if (seqn < 8) {
           grpp->seq1 |= ((ch) << (4 * seqn));
       } else {
           grpp->seq2 |= ((ch) << (4 * seqn));
       }
       seqn++;
   }

   va_end(vl);
}
*/

/**
 * @brief   Configures and activates the ADC peripheral.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
void adc_lld_start(ADCDriver *adcp) {

    if (adcp->state == ADC_STOP) {
        /* Enables the peripheral.*/
#if SAM3X_ADC_USE_ADC1
        if (&ADCD1 == adcp) {
            pmc_enable_periph_clk(ID_ADC);
            adc_configure(adcp);
        }
#endif /* SAM3X_ADC_USE_ADC1 */
        nvicEnableVector(ADC_IRQn, CORTEX_PRIORITY_MASK(SAM3X_ADC_IRQ_PRIORITY));
    }
    /* Configures the peripheral.*/

}

/**
 * @brief   Deactivates the ADC peripheral.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
void adc_lld_stop(ADCDriver *adcp) {

    if (adcp->state == ADC_READY) {
        /* Resets the peripheral.*/

        /* Disables the peripheral.*/
#if SAM3X_ADC_USE_ADC1
        if (&ADCD1 == adcp) {
            pmc_disable_periph_clk(ID_ADC);
            nvicDisableVector(ADC_IRQn);
        }
#endif /* SAM3X_ADC_USE_ADC1 */
    }
}

/**
 * @brief   Starts an ADC conversion.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
extern void testdebug(const char *fmt, ...);

#define printf(...) testdebug(__VA_ARGS__)

void adc_lld_start_conversion(ADCDriver *adcp) {

    (void)adcp;

    /* set mode register */
    adcd->ADC_MR = adcp->mr;
    /*
    //DEBUG: something wrong, pdc works only with RCR=1 ?!?!
    // set up group conversion
    if (adcp->grpp->num_channels > 1) {
        // depth must be even
        adcd->ADC_MR |= ADC_MR_USEQ;
        adcd->ADC_SEQR1 = adcp->grpp->seq1;
        adcd->ADC_SEQR2 = adcp->grpp->seq2;
        // set up pdc with half buffer support
        adcd->ADC_RPR = (uint32_t) adcp->samples;
        adcd->ADC_RCR = adcp->grpp->num_channels * adcp->depth;
        adcd->ADC_RNPR = 0;//(uint32_t) (adcp->samples + adcd->ADC_RCR);
        adcd->ADC_RNCR = 0;//adcp->grpp->num_channels * adcp->depth;
    } 
    else */
    {
        //single pdc
        adcd->ADC_RPR = (uint32_t) adcp->samples;
        adcd->ADC_RCR = adcp->depth;
        adcd->ADC_RNPR = 0;
        adcd->ADC_RNCR = 0;
        //printf("single channel %x samples @ %x/%i\n",adcp->grpp->channels,adcp->samples,adcp->depth);
    }
    /* disable all channels */
    adcd->ADC_CHDR = 0xffff;
    /* set active channels */
    adcd->ADC_CHER = adcp->grpp->channels;

    // enable pdc interrupt
    adcd->ADC_IER = ADC_IER_RXBUFF | ADC_IER_ENDRX;
    //adcd->ADC_IER = ADC_IER_DRDY;

    /* enable pdc */
    adcd->ADC_PTCR = ADC_PTCR_RXTEN;

    sample_pos = 0;

    /* start */
    if (adcp->config->trigger) {
        //TODO: add trigger
    } else {
        adcd->ADC_MR |= ADC_MR_FREERUN_ON;
        adcd->ADC_CR = ADC_CR_START;
    }
}

/**
 * @brief   Stops an ongoing conversion.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object
 *
 * @notapi
 */
void adc_lld_stop_conversion(ADCDriver *adcp) {

    (void)adcp;

    /* stop adc */
    adcd->ADC_CR = ADC_CR_SWRST;
    /* disable pdc interrupt */
    adcd->ADC_IDR = ADC_IER_RXBUFF | ADC_IER_ENDRX | ADC_IER_DRDY;
    //disable all channels
    adcd->ADC_CHDR = 0xffff;
    /* disable pdc */
    adcd->ADC_RPR = 0;
    adcd->ADC_RCR = 0;
    adcd->ADC_RNCR = adcd->ADC_RNPR = 0;
    adcd->ADC_PTCR = 0;
}

#endif /* HAL_USE_ADC */

/** @} */

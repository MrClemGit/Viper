#include "vhal.h"
#include "hal.h"
#include "vhal_gpio.h"
#include "vhal_irqs.h"
#include "vhal_common.h"
#include "vhal_tim.h"

#include "sam3x8e.h"

#define TICKS_PER_MUSEC ((_system_frequency)/1000000)

TIM_CHAN_TypeDef *const tim_l[TIMNUM * CHANNUM] STORED = {&TIM1, &TIM2, &TIM3, &TIM4, &TIM5, &TIM6, &TIM7, &TIM8, &TIM9};

#define TIM(id)  ((id < TIMNUM * CHANNUM) ? tim_l[id] : NULL)

typedef struct {
    htmFn fn;
    void *args;
    uint8_t oneshot;
    vhalIcuConf *conf;
    uint8_t icu_on;
    uint8_t started;
    uint16_t events;
    uint32_t last;
    uint32_t twrc;
} callback_TypeDef;

callback_TypeDef tim_c[TIMNUM * CHANNUM];

uint32_t _timeCalcRegisters(uint32_t time, uint32_t *clock_selection, uint32_t *register_c, uint32_t desired_clks) {
    // Delay in ticks
    int32_t ticks = GET_TIME_MICROS(time) * TICKS_PER_MUSEC;
    *clock_selection = desired_clks;
    uint32_t prescale_factor = 2 << (2 * (*clock_selection));
    *register_c = ticks / prescale_factor;

    return VHAL_OK;
}

uint32_t _timEnablePeriphClock(uint32_t tm, uint32_t enable) {
    switch (tm) {
        case 0:
            if (enable)
                PMC->PMC_PCER0 |= PMC_PCER0_PID27;
            else
                PMC->PMC_PCER0 &= PMC_PCER0_PID27;
            break;
        case 1:
            if (enable)
                PMC->PMC_PCER0 |= PMC_PCER0_PID28;
            else
                PMC->PMC_PCER0 &= PMC_PCER0_PID28;
            break;
        case 2:
            if (enable)
                PMC->PMC_PCER0 |= PMC_PCER0_PID29;
            else
                PMC->PMC_PCER0 &= PMC_PCER0_PID29;
            break;
        case 3:
            if (enable)
                PMC->PMC_PCER0 |= PMC_PCER0_PID30;
            else
                PMC->PMC_PCER0 &= PMC_PCER0_PID30;
            break;
        case 4:
            if (enable)
                PMC->PMC_PCER0 |= PMC_PCER0_PID31;
            else
                PMC->PMC_PCER0 &= PMC_PCER0_PID31;
            break;
        case 5:
            if (enable)
                PMC->PMC_PCER1 |= PMC_PCER1_PID32;
            else
                PMC->PMC_PCER1 &= PMC_PCER1_PID32;
            break;
        case 6:
            if (enable)
                PMC->PMC_PCER1 |= PMC_PCER1_PID33;
            else
                PMC->PMC_PCER1 &= PMC_PCER1_PID33;
            break;
        case 7:
            if (enable)
                PMC->PMC_PCER1 |= PMC_PCER1_PID34;
            else
                PMC->PMC_PCER1 &= PMC_PCER1_PID34;
            break;
        case 8:
            if (enable)
                PMC->PMC_PCER1 |= PMC_PCER1_PID35;
            else
                PMC->PMC_PCER1 &= PMC_PCER1_PID35;
            break;
        default:
            return VHAL_GENERIC_ERROR;
    }
    return VHAL_OK;
}

TIM_TypeDef *_correspondingTimerModule(uint32_t tm) {
    switch (tm / 3) {
        case 0:
            return MODULE_TIM1;
        case 1:
            return MODULE_TIM2;
        case 2:
            return MODULE_TIM3;
        default:
            return NULL;
    }
}

int _enableCorrespondingTimerIrq(uint32_t tm, uint32_t enable) {
    switch (tm / 3) {
        case 0:
            vhalIrqClearPending(TC0_IRQn);
            if (enable)
                vhalIrqEnable(TC0_IRQn);
            else
                vhalIrqDisable(TC0_IRQn);
            return VHAL_OK;
        case 1:
            vhalIrqClearPending(TC1_IRQn);
            if (enable)
                vhalIrqEnable(TC1_IRQn);
            else
                vhalIrqDisable(TC1_IRQn);
            return VHAL_OK;
        case 2:
            vhalIrqClearPending(TC2_IRQn);
            if (enable)
                vhalIrqEnable(TC2_IRQn);
            else
                vhalIrqDisable(TC2_IRQn);
            return VHAL_OK;
        default:
            return VHAL_GENERIC_ERROR;
    }
}

uint32_t _timSetSimple(uint32_t tm, uint32_t delay, uint8_t oneshot, htmFn fn, void *args) {
    TIM_CHAN_TypeDef *timx = TIM(tm);
    TIM_TypeDef *tim;

    if (timx == NULL)
        return VHAL_GENERIC_ERROR;

    // Enable Peripheral Clock
    if (_timEnablePeriphClock(tm, 1) != VHAL_OK)
        return VHAL_GENERIC_ERROR;

    tim = _correspondingTimerModule(tm);

    if (tim == NULL)
        return VHAL_GENERIC_ERROR;

    tim_c[tm].fn = fn;
    tim_c[tm].args = args;
    tim_c[tm].oneshot = oneshot;
    tim_c[tm].icu_on = FALSE;

    uint32_t prescaler;
    uint32_t register_c;
    _timeCalcRegisters(delay, &prescaler, &register_c, TC_CMR_TIMER_CLOCK1);

    timx->TC_CCR.fields.CLKDIS = 1;

    timx->TC_CMR.fields.capture.TCCLKS = prescaler;
    timx->TC_CMR.fields.capture.CLKI = 0;
    timx->TC_CMR.fields.capture.BURST = 0;
    timx->TC_CMR.fields.capture.LDBSTOP = 0;
    timx->TC_CMR.fields.capture.LDBDIS = 0;
    timx->TC_CMR.fields.capture.ETRGEDG = 0;
    timx->TC_CMR.fields.capture.ABETRG = 0;
    timx->TC_CMR.fields.capture.CPCTRG = 1;
    timx->TC_CMR.fields.capture.LDRA = 0;
    timx->TC_CMR.fields.capture.LDRB = 0;

    timx->TC_CMR.fields.capture.WAVE = 0;

    //vhalIrqClearPending(TC0_IRQn);
    //vhalIrqEnable(TC0_IRQn);
    if (_enableCorrespondingTimerIrq(tm, 1) != VHAL_OK)
        return VHAL_GENERIC_ERROR;

    timx->TC_IER.fields.CPCS = 1;
    timx->TC_IER.fields.LDRAS = 0;
    timx->TC_IER.fields.LDRBS = 0;
    timx->TC_IER.fields.COVFS = 0;

    // Disable Write protection
    tim->TC_WPMR.fields.WPKEY = 0x54494D;
    tim->TC_WPMR.fields.WPEN = 0;

    // Set Register RC
    timx->TC_RC = register_c;

    // Fire up the timer
    //timx->TC_CCR.fields.CLKEN = 1;
    //timx->TC_CCR.fields.SWTRG = 1;
    timx->TC_CCR.value = 0b101;

    return VHAL_OK;
}


uint32_t _timReset(uint32_t tm) {
    TIM_CHAN_TypeDef *timx = TIM(tm);

    if (timx == NULL)
        return VHAL_GENERIC_ERROR;

    // Disable Timer
    timx->TC_CCR.fields.CLKDIS = 1;

    // Disable Peripheral Clock
    if (_timEnablePeriphClock(tm, 0) != VHAL_OK)
        return VHAL_GENERIC_ERROR;

    tim_c[tm].fn = NULL;
    tim_c[tm].args = NULL;
    tim_c[tm].oneshot = FALSE;
    tim_c[tm].icu_on = FALSE;
    tim_c[tm].events = 0;
    tim_c[tm].last = 0;
    tim_c[tm].conf = NULL;

    timx->TC_IER.fields.CPCS = 0;
    _enableCorrespondingTimerIrq(tm, 0);

    return VHAL_OK;
}

/* ========================================================================
    IRQ HANDLERS
   ======================================================================== */


void _timIrq_wrapper(uint8_t tm);

void vhalIrq_TC0(void) {
    _timIrq_wrapper(0);
}

void vhalIrq_TC1(void) {
    _timIrq_wrapper(1);
}

void vhalIrq_TC2(void) {
    _timIrq_wrapper(2);
}

void _timIrq_wrapper(uint8_t index) {
    uint8_t tm = index * 3;
    uint8_t channel = tm;
    TIM_CHAN_TypeDef *timx = NULL;
    TC_SR_TypeDef sr;

    vosSysLockIsr();
    // NB: TC_SR is cleared on read!
    uint8_t i;
    for (i = 0; i < 3; ++ i) {
        channel = tm + i;
        sr = TIM(channel)->TC_SR;
        if (sr.fields.CPCS)
            timx = TIM(channel);
        else if (sr.fields.LDRAS || sr.fields.LDRBS)
            timx = TIM(channel);
        if (timx != NULL)
            break;
    }


    if (timx == NULL)
        goto end_fn;

    vosSysUnlockIsr();

    if (tim_c[channel].icu_on == FALSE) {
        // Timer elapsed
        vosSysLockIsr();
        if (tim_c[channel].oneshot) {
            vosSysUnlockIsr();
            timx->TC_CCR.fields.CLKDIS = 1;
            vosSysLockIsr();
        }
        if (*tim_c[channel].fn) {
            vosSysUnlockIsr();
            if (tim_c[channel].fn != NULL)
                (*tim_c[channel].fn)(channel, tim_c[channel].args);
            vosSysLockIsr();
        }
    }
    else {
        // ICU event
        vosSysLockIsr();
        if (tim_c[channel].conf != NULL) {
            if (sr.fields.LDRAS == 1) {
                // Register A was loaded -> New pulse started
                if (!tim_c[channel].started) {
                    timx->TC_RC = tim_c[channel].twrc;
                    tim_c[channel].started = TRUE;
                } else {
                    uint32_t val = timx->TC_RA;
                    val -= tim_c[channel].last;
                    val /= (_system_frequency / (2 * 1000000)); //TODO: remove 2 and multiply by prescaler
                    if (tim_c[channel].events < tim_c[channel].conf->bufsize && tim_c[channel].conf->buffer)
                        tim_c[channel].conf->buffer[tim_c[channel].events] = val;
                    else goto end_icu;
                    if (tim_c[channel].conf->callback) {
                        vosSysUnlockIsr();
                        tim_c[channel].conf->callback( ICU_CFG_GET_TRIGGER(tim_c[channel].conf->cfg), val, tim_c[channel].events);
                        vosSysLockIsr();
                    }
                    tim_c[channel].events++;
                }
                tim_c[channel].last = timx->TC_RA;
            } else if (sr.fields.LDRBS == 1) {
                // Register B was loaded -> Pulse finished
                uint32_t val = timx->TC_RB;
                val -= tim_c[channel].last;
                val /= (_system_frequency / (2 * 1000000)); //TODO: remove 2 and multiply by prescaler
                if (tim_c[channel].events < tim_c[channel].conf->bufsize && tim_c[channel].conf->buffer)
                    tim_c[channel].conf->buffer[tim_c[channel].events] = val;
                else goto end_icu;
                tim_c[channel].last = timx->TC_RB;
                if (tim_c[channel].conf->callback) {
                    vosSysUnlockIsr();
                    tim_c[channel].conf->callback( !ICU_CFG_GET_TRIGGER(tim_c[channel].conf->cfg), val, tim_c[channel].events);
                    vosSysLockIsr();
                }
                tim_c[channel].events++;
            }
            if (sr.fields.CPCS == 1 && tim_c[channel].started) {
                // Time window expired
end_icu:
                // Invoking Callback
                timx->TC_CCR.fields.CLKDIS = 1;
                timx->TC_IDR.value=0xff;
                if (tim_c[channel].conf->bufsize > tim_c[channel].events)
                    tim_c[channel].conf->bufsize = tim_c[channel].events;
                if (tim_c[channel].conf->endcbk) {
                    vosSysUnlockIsr();
                    tim_c[channel].conf->endcbk(tim_c[channel].conf);
                    vosSysLockIsr();
                } else {
                    vosThResumeIsr(tim_c[channel].conf->args);
                }
            }
        }
    }
end_fn:
    vosSysUnlockIsr();
}


int vhalInitTIM(void *data) {
    (void)data;
    // Nothing to do...
    return VHAL_OK;
}


int _enablePWMPin(int vpin) {
    int port = PIN_PORT_NUMBER(vpin);
    int pad = PIN_PAD(vpin);
    //mode is not important, just set status to PRPH_PWM
    vhalPinSetToPeripheral(vpin, PRPH_PWM, PAL_MODE_OUTPUT_PUSHPULL);
    //All Arduino PWM pins go to B
    //TODO: make it generic
    switch (port) {
        case PORT_A:
            PIOA->PIO_PDR |= (0x1 << pad);
            PIOA->PIO_ABSR |= (0x1 << pad);
            break;
        case PORT_B:
            PIOB->PIO_PDR |= (0x1 << pad);
            PIOB->PIO_ABSR |= (0x1 << pad);
            break;
        case PORT_C:
            PIOC->PIO_PDR |= (0x1 << pad);
            PIOC->PIO_ABSR |= (0x1 << pad);
            break;
        default:
            return VHAL_GENERIC_ERROR;
    }
    return VHAL_OK;
}

int _setPWMPin(int vpin, uint32_t clk_src, uint32_t cprd, uint32_t cdty) {
    uint32_t channel = PIN_CLASS_DATA0(vpin);

    // Disable the channel first
    PWM->PWM_DIS = (0x1 << channel);
    // Disable Write Protection for PWM
    PWM->PWM_WPCR = 0x50574DFC;
    // CLOCK SELECTION
    PWM->PWM_CH_NUM[channel].PWM_CMR = clk_src;
    // PERIOD
    PWM->PWM_CH_NUM[channel].PWM_CPRD = cprd;
    // DUTY CYCLE
    PWM->PWM_CH_NUM[channel].PWM_CDTY = cdty;

    // Enable Channel
    PWM->PWM_ENA = (0x1 << channel);

    return VHAL_OK;
}


int _calcPWMParams(uint32_t period, uint32_t pulse, uint32_t *clk_src, uint32_t *cprd, uint32_t *cdty) {
    if (period <= pulse)
        return VHAL_GENERIC_ERROR;

    uint32_t period_in_us = GET_TIME_MICROS(period);
    uint32_t pulse_in_us = GET_TIME_MICROS(pulse);
    //minimum needed prescaler
    uint32_t psc = (period_in_us * TICKS_PER_MUSEC) / 65536;
    uint32_t mainpsc = 0;
    uint32_t prescaler = (0x1 << mainpsc);
    while (psc > 0) {
        psc = psc / 2;
        prescaler = prescaler * 2;
        mainpsc++;
    }

    if ((period_in_us * TICKS_PER_MUSEC) > (prescaler * 65536)) {
        prescaler = prescaler * 2;
        mainpsc++;
    }

    *cprd = (period_in_us * TICKS_PER_MUSEC) / (prescaler);
    *cdty = (pulse_in_us * TICKS_PER_MUSEC) / (prescaler);
    *clk_src = mainpsc;

    if (mainpsc > 10) //not greater than 1024
        return -1;

    return VHAL_OK;
}


//extern void testdebug(const char *fmt, ...);
//#define printf(...) testdebug(__VA_ARGS__)
#define printf(...)

int vhalPwmStart(int vpin, uint32_t period, uint32_t pulse) {

    vosSysLock();
    if (!PIN_HAS_PRPH(vpin, PRPH_PWM)) {
        vosSysUnlock();
        printf("invalid pin\n");
        return VHAL_INVALID_PIN;
    }

    if ((period == 0 || pulse == 0 || period < pulse)) {
        uint32_t channel = PIN_CLASS_DATA0(vpin);
        // Check if PWM channel is active
        if (PWM_CHANNEL_IS_ACTIVE(channel)) {
            PWM->PWM_DIS = 0x1 << channel;

            // Disable Peripheral Clock if every PWM Channel is disabled
            if ((PWM->PWM_SR & 0xFF) == 0)
                PMC->PMC_PCER1 &= ~PMC_PCER1_PID36;
        }
        vosSysUnlock();
        printf("return 0\n");
        return VHAL_OK;
    }

    uint32_t clk_src = PWM_CMR_CPRE_MCK;
    uint32_t cprd;
    uint32_t cdty;

    if (_calcPWMParams(period, pulse, &clk_src, &cprd, &cdty) != VHAL_OK) {
        printf("period %i pulse %i => PSC %i PRD %i DTY %i\n", GET_TIME_MICROS(period), GET_TIME_MICROS(pulse), clk_src, cprd, cdty);
        printf("err\n");
        return VHAL_GENERIC_ERROR;
    }

    printf("period %i pulse %i => PSC %i PRD %i DTY %i\n", GET_TIME_MICROS(period), GET_TIME_MICROS(pulse), clk_src, cprd, cdty);
    // Enable Peripheral Clock for PWM
    PMC->PMC_PCER1 |= PMC_PCER1_PID36;

    //vhalPinSetToPeripheral(vpin, PRPH_PWM, SAM3X_PIN_PR(vpin) | PAL_MODE_OUTPUT_PUSHPULL);

    if (_enablePWMPin(vpin) != VHAL_OK) {
        vosSysUnlock();
        return VHAL_GENERIC_ERROR;
    }

    //TODO: handle H channels
    //cdty = (PIN_CLASS_DATA2(vpin)==1) ? cdty:(period-cdty);
    if (_setPWMPin(vpin, clk_src, cprd, cdty) != VHAL_OK) {
        vosSysUnlock();
        return VHAL_GENERIC_ERROR;
    }

    vosSysUnlock();
    return VHAL_OK;
}


int vhalIcuStart(int vpin, vhalIcuConf *conf) {
    vosSysLock();
    if (!PIN_HAS_PRPH(vpin, PRPH_ICU)) {
        vosSysUnlock();
        return VHAL_INVALID_PIN;
    }

    // Pull-up or Pull-down (Only Pull-up is supported)
    uint32_t icu_input = ICU_CFG_GET_INPUT(conf->cfg);

    uint32_t trigger = ICU_CFG_GET_TRIGGER(conf->cfg);

    // Obtain the corresponding TC channel and peripheral data
    int port = PIN_PORT_NUMBER(vpin);
    int pad = PIN_PAD(vpin);
    uint32_t channel = PIN_CLASS_DATA0(vpin);
    uint32_t pio_prph = PIN_CLASS_DATA1(vpin);

    // Obtain the corresponding Timer channel
    vosSysLock();
    int tmh = GET_PERIPHERAL_ID(htm, channel);

    if (conf == NULL) {
        _timReset(tmh);
        vosSysUnlock();
        return VHAL_OK;
    }

    if (conf->time_window == 0) {
        _timReset(tmh);
        vosSysUnlock();
        return VHAL_OK;
    }

    TIM_CHAN_TypeDef *timx = TIM(tmh);

    if (timx->TC_SR.fields.CLKSTA != 0) {
        // The timer channel is already busy
        vosSysUnlock();
        return VHAL_GENERIC_ERROR;
    }

    TIM_TypeDef *tim;

    // Enable Peripheral Clock
    if (_timEnablePeriphClock(tmh, 1) != VHAL_OK) {
        vosSysUnlock();
        return VHAL_GENERIC_ERROR;
    }

    tim = _correspondingTimerModule(tmh);

    if (tim == NULL) {
        vosSysUnlock();
        return VHAL_GENERIC_ERROR;
    }

    // Enable Peripheral A/B on pin
    // TODO: get port register directly from vpin
    switch (port) {
        case PORT_A:
            PIOA->PIO_PDR |= (0x1 << pad);
            PIOA->PIO_ABSR = (PIOA->PIO_ABSR & ~(1 << pad)) | (pio_prph << pad);
            break;
        case PORT_B:
            PIOB->PIO_PDR |= (0x1 << pad);
            PIOB->PIO_ABSR |= (PIOB->PIO_ABSR & ~(1 << pad)) | (pio_prph << pad);
            break;
        case PORT_C:
            PIOC->PIO_PDR |= (0x1 << pad);
            PIOC->PIO_ABSR |= (PIOC->PIO_ABSR & ~(1 << pad)) | (pio_prph << pad);
            break;
        case PORT_D:
            PIOD->PIO_PDR |= (0x1 << pad);
            PIOD->PIO_ABSR |= (PIOD->PIO_ABSR & ~(1 << pad)) | (pio_prph << pad);
            break;
        default:
            vosSysUnlock();
            return VHAL_GENERIC_ERROR;
    }

    if (icu_input == ICU_INPUT_PULLUP)
        vhalPinSetToPeripheral(vpin, PRPH_ICU, SAM3X_PIN_PR(vpin) | PAL_MODE_OUTPUT_PUSHPULL | PAL_MODE_INPUT_PULLUP);
    else
        vhalPinSetToPeripheral(vpin, PRPH_ICU, SAM3X_PIN_PR(vpin) | PAL_MODE_OUTPUT_PUSHPULL);

    tim_c[tmh].fn = NULL;
    tim_c[tmh].args = NULL;
    tim_c[tmh].oneshot = FALSE;
    tim_c[tmh].events = 0;
    tim_c[tmh].icu_on = TRUE;
    tim_c[tmh].conf = conf;
    tim_c[tmh].started = FALSE;

    if (!conf->endcbk) {
        //if no end callback given, must save current thread
        conf->args = (void *)vosThCurrent();
    }

    uint32_t prescaler = 0;
    uint32_t register_c = 0;
    _timeCalcRegisters(conf->time_window, &prescaler, &register_c, TC_CMR_TIMER_CLOCK1);

    tim_c[tmh].twrc = register_c;

    timx->TC_CCR.fields.CLKDIS = 1;

    timx->TC_CMR.fields.capture.TCCLKS = prescaler;
    timx->TC_CMR.fields.capture.CLKI = 0;
    timx->TC_CMR.fields.capture.BURST = 0;
    timx->TC_CMR.fields.capture.LDBSTOP = 0;
    timx->TC_CMR.fields.capture.LDBDIS = 0;
    timx->TC_CMR.fields.capture.ETRGEDG = 0;
    timx->TC_CMR.fields.capture.ABETRG = 1;
    timx->TC_CMR.fields.capture.CPCTRG = 1;

    if (trigger == ICU_TRIGGER_HIGH) {
        timx->TC_CMR.fields.capture.LDRA = 1; // RISING EDGE
        timx->TC_CMR.fields.capture.LDRB = 2; // FALLING EDGE
    }
    else {
        timx->TC_CMR.fields.capture.LDRA = 2; // FALLING EDGE
        timx->TC_CMR.fields.capture.LDRB = 1; // RISING EDGE
    }

    timx->TC_CMR.fields.capture.WAVE = 0;

    if (_enableCorrespondingTimerIrq(tmh, 1) != VHAL_OK) {
        vosSysUnlock();
        return VHAL_GENERIC_ERROR;
    }

    timx->TC_IER.fields.CPCS = 1;
    timx->TC_IER.fields.LDRAS = 1;
    timx->TC_IER.fields.LDRBS = 1;
    timx->TC_IER.fields.COVFS = 1;

    // Disable Write protection
    tim->TC_WPMR.fields.WPKEY = 0x54494D;
    tim->TC_WPMR.fields.WPEN = 0;

    // Set Register RC
    timx->TC_RC = register_c;

    // Fire up the timer
    //timx->TC_CCR.fields.CLKEN = 1;
    //timx->TC_CCR.fields.SWTRG = 1;
    timx->TC_CCR.value = 0b101;

    //blocking
    if (!conf->endcbk) {
        vosSysLock();
        vosThSuspend();
    }

    vosSysUnlock();
    return VHAL_OK;
}

int vhalHtmGetFreeTimer() {
    int prphs = PERIPHERAL_NUM(htm);
    int idx;
    int tmh;
    vosSysLock();
    for (idx = 0; idx < prphs; idx++) {
        tmh = GET_PERIPHERAL_ID(htm, idx);
        if (TIM(tmh)->TC_SR.fields.CLKSTA == 0) {
            goto end_fn;
        }
    }
    tmh = VHAL_GENERIC_ERROR;
end_fn:
    vosSysUnlock();
    return tmh;
}

int _set_htm_timer(uint32_t tm, uint32_t delay, htmFn fn, uint8_t oneshot, void *args) {
    if (delay == 0 || fn == NULL)
        return _timReset(tm);
    else
        return _timSetSimple(tm, delay, oneshot, fn, args);
}

int vhalHtmOneShot(uint32_t tm, uint32_t delay, htmFn fn, void *args) {
    return _set_htm_timer(tm, delay, fn, TRUE, args);
}


int vhalHtmRecurrent(uint32_t tm, uint32_t delay, htmFn fn, void *args) {
    return _set_htm_timer(tm, delay, fn, FALSE, args);
}






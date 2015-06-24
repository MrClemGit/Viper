#include "vhal.h"
#include "vhal_gpio.h"
#include "vhal_tim.h"
#include "vhal_irqs.h"
#include "vhal_common.h"


//TODO: add compatibility for all f4 mcu

/*    |TODO|

  - capability map for TIMx (6,7,8? TIM_IT_Update event?)
  - parameter check (range for s, ms, us)
  - TIMx check for capability/state

*/


/* ========================================================================
    TIMER CONSTANTS
   ======================================================================== */

const uint8_t const tim_irqs[TIMNUM] STORED = {
  TIM1_UP_IRQn,
  TIM2_IRQn,
  TIM3_IRQn,
  TIM4_IRQn
};

TIM_TypeDef *const tim_l[TIMNUM] STORED = {TIM1, TIM2, TIM3, TIM4};

#define TIM(x) tim_l[x]
#define TIM_IRQ(x) tim_irqs[x]


uint8_t tim_s[TIMNUM];
void _timICUIrq(uint8_t tm, uint32_t sr);


/* ========================================================================
    TIMER STATUS
   ======================================================================== */


//  0    1     2    3    4    5    6     7
// free     fn           ch1  ch2  ch3   ch4

#define TIM_HTM_ONE_SHOT  0x01
#define TIM_HTM_RECURRENT 0x03
#define TIM_STATUS_PWM    0x05
#define TIM_STATUS_ICU    0x07
#define TIM_CH(x)         (1<<(4+x))
#define TIMER_STATUS(tm) (tim_s[tm]&0x07)
#define TIMER_SET_STATUS(tm,x) tim_s[tm]=(x)
#define TIMER_STATUS_CH_ON(tm,x) tim_s[tm]|=TIM_CH(x)
#define TIMER_STATUS_CH_OFF(tm,x) tim_s[tm]&=~TIM_CH(x)
#define TIMER_STATUS_CH(tm) (tim_s[tm]&0xf0)
#define TIMER_STATUS_HAS_CH(tm,x) (tim_s[tm]&TIM_CH(x))
#define TIMER_STATUS_HAS_ONLY_CH(tm,x) (( (tim_s[tm]&0xf0)>>4 )==(1<<(x)))


#define  TIM_STATUS_USABLE                    0
#define  TIM_STATUS_MARKED                    2
#define  ICU_STATUS_USABLE                    0
#define  ICU_STATUS_RUNNING                   2

#define VP_TOTIM(X) (PIN_CLASS_DATA0(X) -1)
#define VP_TOCHA(X) (PIN_CLASS_DATA1(X) -1)
#define VP_TOAF(X)  (PIN_CLASS_DATA2(X))



//extern void testdebug(const char *fmt, ...);
//#define printf(...) testdebug(__VA_ARGS__)
//#define printf(...)

//extern void vbl_printf_stdout(uint8_t *fmt, ...);
//#define printf(...) vbl_printf_stdout(__VA_ARGS__)
#define printf(...)



typedef struct {
  htmFn fn;
  void *args;
} callback_TypeDef;


typedef struct _icu_struct {
  vhalIcuConf *conf;
  uint16_t count;
  uint16_t last;
  uint16_t updated;
  uint8_t started;
  uint8_t pad;
  uint32_t has_capture;
  uint32_t start_capture;
  GPIO_TypeDef *port; //TODO: this can be removed by using CCnP flipping
} ICU_TypeDef;


//just pointers
ICU_TypeDef *icu_struct[TIMNUM][4];
callback_TypeDef tim_c[TIMNUM];
VSemaphore icu_sem;



/* ========================================================================
    TIMER HANDLING
   ======================================================================== */



void _timFree_timer(uint8_t tm) {
  rcc_disable_tim(tm);
  vhalIrqDisable(TIM_IRQ(tm));
  if (TIM(tm) == TIM1) {
    vhalIrqDisable(TIM1_CC_IRQn);
  } /*else if (TIM(tm)==TIM8){
    vhalIrqDisable(TIM8_CC_IRQn);
  }*/
  TIMER_SET_STATUS(tm, TIM_STATUS_USABLE);
}


void _timFree_channel(uint8_t tm, uint8_t ch) {
  TIMER_STATUS_CH_OFF(tm, ch);
  if (!TIMER_STATUS_CH(tm)) {
    _timFree_timer(tm);
  }
}


void _timReserveAndInit(uint32_t tm, uint8_t status) {
  rcc_enable_tim(tm);
  TIM(tm)->SR = (uint16_t)~TIM_IT_Update;
  TIMER_SET_STATUS(tm, status);
}

#define TICKS_PER_MUSEC ((_system_frequency)/1000000)

int _timeCalcRegisters(uint32_t time, uint32_t *psc, uint32_t *prd, uint32_t desired_psc) {
  uint32_t ticks = GET_TIME_MICROS(time) * TICKS_PER_MUSEC;
  *psc = desired_psc;
  *prd = ticks / (*psc + 1);

  if (*prd <= 0xffff) {
    return 1;
  }
  *psc = ticks / (65536); //minimum required psc
  *prd = ticks / (*psc + 1); //corresponding prd
  if (*prd <= 0xffff)
    return 0;
  return VHAL_GENERIC_ERROR;
}


void _set_timer_base(TIM_TypeDef *timx, uint16_t TIM_Prescaler, uint32_t TIM_Period, uint8_t TIM_RepetitionCounter) {
  uint16_t tmpcr1 = 0;

  tmpcr1 = timx->CR1;

  if ((timx == TIM1) || (timx == TIM8) ||
      (timx == TIM2) || (timx == TIM3) ||
      (timx == TIM4) || (timx == TIM5)) {
    /* Select the Counter Mode */
    tmpcr1 &= (uint16_t)(~(TIM_CR1_DIR | TIM_CR1_CMS));
    tmpcr1 |= (uint32_t)TIM_CounterMode_Up;
  }

  if ((timx != TIM6) && (timx != TIM7)) {
    /* Set the clock division */
    tmpcr1 &=  (uint16_t)(~TIM_CR1_CKD);
    //set to 0
    //tmpcr1 |= (uint32_t)TIM_ClockDivision;
  }

  timx->CR1 = tmpcr1;

  /* Set the Autoreload value */
  timx->ARR = TIM_Period ;

  /* Set the Prescaler value */
  timx->PSC = TIM_Prescaler;

  if ((timx == TIM1) || (timx == TIM8)) {
    /* Set the Repetition Counter value */
    timx->RCR = TIM_RepetitionCounter;
  }

  /* Generate an update event to reload the Prescaler
     and the repetition counter(only for TIM1 and TIM8) value immediately */
  timx->EGR = TIM_PSCReloadMode_Immediate;
}



/* ========================================================================
    IRQ HANDLERS
   ======================================================================== */


void _timIrq_wrapper(uint8_t tm);

void vhalIrq_TIM4(void) {
  _timIrq_wrapper(3);
}
void vhalIrq_TIM3(void) {
  _timIrq_wrapper(2);
}
void vhalIrq_TIM2(void) {
  _timIrq_wrapper(1);
}

void vhalIrq_TIM1(void) {
  _timIrq_wrapper(0);
}

void vhalIrq_TIM1_CC(void) {
  _timIrq_wrapper(0);
}


#define SR_CAPTURE (TIM_SR_CC1IF|TIM_SR_CC2IF|TIM_SR_CC3IF|TIM_SR_CC4IF)
#define SR_UPDATE (TIM_SR_UIF)

volatile int icuflag = 0;
void _timIrq_wrapper(uint8_t tm) {
  //get status masked
  uint32_t sr = TIM(tm)->SR & TIM(tm)->DIER;

  vosSysLockIsr();
  if (TIMER_STATUS(tm) == TIM_STATUS_ICU  && (sr & (SR_UPDATE | SR_CAPTURE))) {
    _timICUIrq(tm, sr);
  } else if (sr & SR_UPDATE) {
    if (TIMER_STATUS(tm) == TIM_HTM_ONE_SHOT || TIMER_STATUS(tm) == TIM_HTM_RECURRENT) {
      if (*tim_c[tm].fn) {
        vosSysUnlockIsr();
        (*tim_c[tm].fn)(tm, tim_c[tm].args);  /* execute callback */
        vosSysLockIsr();
      }
      if (TIMER_STATUS(tm) == TIM_HTM_ONE_SHOT) { /* free TIMx */
        _timFree_timer(tm);
      }
    }
  }
  TIM(tm)->SR = ~sr;
  vosSysUnlockIsr();
}

/* ========================================================================
    PWM CONFIG
   ======================================================================== */

struct _pwmconfig {
  uint16_t ccer;
  uint16_t ccmr;
  uint16_t cr2;
};


const struct _pwmconfig const pwmch_reg[] STORED = {
  { ~(TIM_CCER_CC1E | TIM_CCER_CC1P | TIM_CCER_CC1NP | TIM_CCER_CC1NE),
    ~(TIM_CCMR1_OC1M | TIM_CCMR1_CC1S | TIM_CCMR1_OC1PE),
    ~(TIM_CR2_OIS1 | TIM_CR2_OIS1N)
  },
  { ~(TIM_CCER_CC2E | TIM_CCER_CC2P | TIM_CCER_CC2NP | TIM_CCER_CC2NE),
    ~(TIM_CCMR1_OC2M | TIM_CCMR1_CC2S | TIM_CCMR1_OC2PE),
    ~(TIM_CR2_OIS2 | TIM_CR2_OIS2N)
  },
  { ~(TIM_CCER_CC3E | TIM_CCER_CC3P | TIM_CCER_CC3NP | TIM_CCER_CC3NE),
    ~(TIM_CCMR2_OC3M | TIM_CCMR2_CC3S | TIM_CCMR2_OC3PE),
    ~(TIM_CR2_OIS3 | TIM_CR2_OIS3N)
  },
  { ~(TIM_CCER_CC4E | TIM_CCER_CC4P),
    ~(TIM_CCMR2_OC4M | TIM_CCMR2_CC4S | TIM_CCMR2_OC4PE),
    ~(TIM_CR2_OIS4)
  },
};

void _timOCxInit(uint8_t ch, TIM_TypeDef *timx, uint32_t period) {
  uint16_t tmpccmrx = 0, tmpccer = 0, tmpcr2 = 0;
  tmpccer = timx->CCER;
  tmpcr2 =  timx->CR2;
  tmpccmrx = (ch < 2) ? timx->CCMR1 : timx->CCMR2;

  tmpccer &= pwmch_reg[ch].ccer;
  tmpccmrx &= pwmch_reg[ch].ccmr;
  tmpccer |= (TIM_OutputNState_Enable | TIM_OutputState_Enable) << (4 * ch); //can be or'ed with OCPOLARITY
  tmpccmrx |= (TIM_OCMode_PWM1 | TIM_OCPreload_Enable) << (8 * (ch % 2));
  timx->CCR[ch] = period;

  if ((timx == TIM1) || (timx == TIM8)) {
    tmpcr2 &= pwmch_reg[ch].cr2;
    tmpcr2 |= (TIM_OCIdleState_Set) << (2 * ch);
  }

  timx->CR2 = tmpcr2;
  if (ch < 2)
    timx->CCMR1 = tmpccmrx;
  else
    timx->CCMR2 = tmpccmrx;
  timx->CCER = tmpccer;
  timx->EGR |= ((uint8_t)0x01);
}


/* END PRIVATE fun*/




int vhalInitTIM(void *data) {
  (void)data;
  memset(tim_s, TIM_STATUS_USABLE, sizeof(tim_s));
  memset(icu_struct, 0, sizeof(icu_struct));
  icu_sem = vosSemCreate(1);
  return 0;
}


/* ========================================================================
    PWM
   ======================================================================== */

#define GPIOA_BASE            (APB2PERIPH_BASE + 0x0800)
#define GPIOB_BASE            (APB2PERIPH_BASE + 0x0C00)
#define GPIOA               ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB               ((GPIO_TypeDef *) GPIOB_BASE)


int vhalPwmStart(int vpin, uint32_t period, uint32_t pulse) {
  if (!PIN_HAS_PRPH(vpin, PRPH_PWM)) {
    printf("NOT A PWM VPIN\n");
    return VHAL_INVALID_PIN;
  }
  uint32_t tm = VP_TOTIM(vpin);
  uint32_t ch = VP_TOCHA (vpin);

  TIM_TypeDef *timx = TIM(tm);



  if ((period == 0 || pulse == 0 || period < pulse)) {
    if (TIMER_STATUS_HAS_CH(tm, ch)) {
      timx->CCER &= ~(TIM_CCER_CC1E << (4 * ch)); //disable channel
      _timFree_channel(tm, ch);
    }
    return VHAL_OK;
  }

  if (!(TIMER_STATUS(tm) == TIM_STATUS_PWM  || TIMER_STATUS(tm) == TIM_STATUS_USABLE)
      || !PIN_FREE_OR_ASSIGNED_TO(vpin, PRPH_PWM)) {
    printf("NOT USABLE AS PWM %x %i\n", vpin, TIMER_STATUS(tm));
    return VHAL_INVALID_PIN;
  }


  uint32_t tim_period = 0;
  uint32_t pwm_period = 0;
  uint32_t prescaler = 0;
  _timeCalcRegisters(period, &prescaler, &tim_period, 0);
  //the prescaler is the same because pwm_period is pulse_ticks/(prescaler+1)+1
  printf("PSC %i, PRD %i\n", prescaler, tim_period);

  if (!(TIMER_STATUS_HAS_ONLY_CH(tm, ch))) {
    if (TIMER_STATUS_CH(tm) && timx->ARR != tim_period) {
      printf("CURRENT PWM HAS DIFFERENT PERIOD\n");
      return VHAL_HARDWARE_STATUS_ERROR;
    }
  }
  _timeCalcRegisters(pulse, &prescaler, &pwm_period, prescaler);
  printf("PSC %i, PRD %i\n", prescaler, pwm_period);

  //pwm_periods[tm] = period;

  if (!TIMER_STATUS_CH(tm) || TIMER_STATUS_HAS_ONLY_CH(tm, ch)) {
    //first init
    _timReserveAndInit(tm, TIM_STATUS_PWM);
    _set_timer_base(timx, prescaler, tim_period, 0);
    timx->DIER &= ~TIM_IT_Update; //disable update interrupt
    timx->CNT = 0;
    timx->SR = 0;
  }

  TIMER_STATUS_CH_ON(tm, ch);
  //timers are remapped in boards.h
  //TODO: allow on the fly remapping based on vpin
  vhalPinSetToPeripheral(vpin, PRPH_PWM, PIN_PARAMS(PINPRM_MODE_50MHZ, PINPRM_CNF_ALTERNATE_PP, 0, 0, 0));
  //vhalPinSetToPeripheral(vpin, PRPH_PWM, ALTERNATE_FN(VP_TOAF(vpin)) | STM32_PUDR_NOPULL | STM32_OSPEED_HIGHEST | STM32_OTYPE_PUSHPULL);

  /* Set the ARR Preload Bit */
  timx->CR1 |= TIM_CR1_ARPE;

  _timOCxInit(ch, timx, pwm_period);

  if (timx == TIM1 || timx == TIM8) {
    timx->BDTR |= TIM_BDTR_MOE;  /* Enable the TIM Main Output (TIM_CtrlPWMOutputs in ST stdlib)*/
  }
  timx->CR1 |= TIM_CR1_CEN;    /* Enable the TIM Counter  (TIM_Cmd in ST stdlib) */

  printf("\n\n");

  return VHAL_OK;
}


/* ========================================================================
    ICU
   ======================================================================== */


struct _icuconfig {
  uint16_t ccer;
  uint16_t ccmr;
};

const struct _icuconfig const icuregs[] STORED = {
  {(uint16_t)~(TIM_CCER_CC1P | TIM_CCER_CC1NP), (uint16_t)~(TIM_CCMR1_CC1S | TIM_CCMR1_IC1F)},
  {(uint16_t)~(TIM_CCER_CC2P | TIM_CCER_CC2NP), (uint16_t)~(TIM_CCMR1_CC2S | TIM_CCMR1_IC2F)},
  {(uint16_t)~(TIM_CCER_CC3P | TIM_CCER_CC3NP), (uint16_t)~(TIM_CCMR2_CC3S | TIM_CCMR2_IC3F)},
  {(uint16_t)~(TIM_CCER_CC4P | TIM_CCER_CC4NP), (uint16_t)~(TIM_CCMR2_CC4S | TIM_CCMR2_IC4F)},
};


void TIM_ICInit(TIM_TypeDef *timx, uint32_t ch, uint32_t polarity) {

  uint16_t ccmr, ccer;

  ccmr = (ch < 2) ? timx->CCMR1 : timx->CCMR2;
  timx->CCER &= ~(TIM_CCER_CC1E << (4 * ch)); //disable channel
  ccer = timx->CCER;
  ccer &= icuregs[ch].ccer;
  ccmr &= icuregs[ch].ccmr;
  ccmr |= (TIM_ICSelection_DirectTI | (/*filter*/0 << 4) | TIM_ICPSC_DIV1) << (8 * (ch % 2));
  ccer |= (TIM_CCER_CC1E << (4 * ch)) | (polarity << (4 * ch));
  if (ch < 2) {
    timx->CCMR1 = ccmr;
  } else {
    timx->CCMR2 = ccmr;
  }
  timx->CCER = ccer;

}


#define ICU_END_CAPTURE() {\
    _timFree_channel(tm, ch);\
    timx->DIER &= ~(TIM_IT_CC1 << ch);\
    icu->conf->bufsize = icu->count; /*save read timings*/  \
    icu->conf->time_window = TIME_U(icu->conf->time_window,MICROS); \
    if (icu->conf->endcbk) { \
      vosSysUnlockIsr(); \
      icu->conf->endcbk(icu->conf); \
      vosSysLockIsr(); \
    } else { \
      /*args is used to store current thread for blocking calls*/ \
      vosThResumeIsr(icu->conf->args); \
    }\
  }

#define IS_ELAPSED(icu)   ((!icu->has_capture) ? (timx->CNT-icu->start_capture>icu->conf->time_window):((icu->has_capture-icu->start_capture)>icu->conf->time_window))


void _timICUIrq(uint8_t tm, uint32_t sr) {
  TIM_TypeDef *timx = TIM(tm);
  uint16_t ch;
  uint16_t now;
  uint32_t val = 0;


  if (sr & SR_UPDATE) {
    //printf("tm %i sr %x\n",tm,timx->DIER);
    for (ch = 0; ch < 4; ch++) {
      if (!TIMER_STATUS_HAS_CH(tm, ch))
        continue;
      ICU_TypeDef *icu = icu_struct[tm][ch];
      if (icu->started) {
        icu->updated++;
        icu->has_capture += timx->ARR;
        if (IS_ELAPSED(icu)) {
          //time_window elapsed, end capture
          ICU_END_CAPTURE();
          continue;
        }
      }
    }
  }

  if (sr & SR_CAPTURE) {
    //printf("tm %i cp %x\n",tm,sr);
    icuflag++;
    for (ch = 0; ch < 4; ch++) {
      if ( (sr & (TIM_SR_CC1IF << ch)) && TIMER_STATUS_HAS_CH(tm, ch)) {
        ICU_TypeDef *icu = icu_struct[tm][ch];
        now = timx->CCR[ch];
        if (icu->started) {
          if (icu->updated) {
            val = ((now + timx->ARR * (icu->updated) - icu->last));
          } else {
            val = ((uint16_t)(now - icu->last));
          }
          val = (val * (timx->PSC + 1)) / (_system_frequency / 1000000);//micros
          if (icu->conf->buffer && icu->count < icu->conf->bufsize) {
            icu->conf->buffer[icu->count] = val;
          }
          if ( (icu->conf->buffer && icu->count >= icu->conf->bufsize) || IS_ELAPSED(icu)) {
            ICU_END_CAPTURE();
            continue;
          }
          if (icu->conf->callback) {
            vosSysUnlockIsr();
            icu->conf->callback(  (icu->port->IDR >> icu->pad) & 1, val, icu->count);
            vosSysLockIsr();
          }
          icu->count++;
          icu->last = now;
        } else {
          //start!
          icu->last = now;
          icu->started = 1;
          icu->has_capture = 0;
          icu->start_capture = timx->CNT;
        }
        icu->updated = 0;
        timx->CCER^=(2<<(ch*4)); //no bothedge mode, flip the CC(ch)P bit in CCER 
      }
    }
  }
}


int vhalIcuStart(int vpin, vhalIcuConf *conf) {
  int ret = VHAL_GENERIC_ERROR;
  vosSemWait(icu_sem);

  if (!PIN_HAS_PRPH(vpin, PRPH_ICU)) {
    printf("NOT A ICU VPIN\n");
    ret = VHAL_INVALID_PIN;
    goto _ret;
  }

  uint32_t tm = VP_TOTIM(vpin);
  uint32_t ch = VP_TOCHA (vpin);

  if (!(TIMER_STATUS(tm) == TIM_STATUS_ICU  || TIMER_STATUS(tm) == TIM_STATUS_USABLE)
      || !PIN_FREE_OR_ASSIGNED_TO(vpin, PRPH_ICU)) {
    printf("NOT USABLE AS ICU %x %i\n", vpin, TIMER_STATUS(tm));
    ret = VHAL_INVALID_PIN;
    goto _ret;
  }

  if (TIMER_STATUS_HAS_CH(tm, ch)) {
    printf("ALREADY CAPTURING\n");
    ret = VHAL_HARDWARE_STATUS_ERROR;
    goto _ret;
  }

  TIM_TypeDef *timx = TIM(tm);
  uint32_t prescaler = _system_frequency / 1000000 - 1, period = 65535;
  //_timeCalcRegisters(conf->time_window, &prescaler, &period, 0);

  if (!TIMER_STATUS_CH(tm)) {
    //first init
    _timReserveAndInit(tm, TIM_STATUS_ICU);
    _set_timer_base(timx, prescaler, period, 0);
    printf("PSC %i, PRD %i\n", prescaler, period);
    timx->CNT = 0;
    timx->SR = 0;
  } else {
    if (prescaler != timx->PSC || period != timx->ARR) {
      printf("CURRENT ICU HAS DIFFERENT PERIOD\n");
      ret = VHAL_HARDWARE_STATUS_ERROR;
      goto _ret;
    }
  }

  TIMER_STATUS_CH_ON(tm, ch);

  //cleanup leftovers and alloc as needed
  period = ch;
  for (ch = 0; ch < 4; ch++) {
    if (!TIMER_STATUS_HAS_CH(tm, ch) && icu_struct[tm][ch]) {
      gc_free(icu_struct[tm][ch]);
      icu_struct[tm][ch] = NULL;
    } else if (TIMER_STATUS_HAS_CH(tm, ch) && !icu_struct[tm][ch]) {
      icu_struct[tm][ch] = (ICU_TypeDef *)gc_malloc(sizeof(ICU_TypeDef));
    }
  }
  ch = period;
  //new channel: since this function is blocking we alloc memory everytime
  ICU_TypeDef *icu_s = icu_struct[tm][ch];
  //icu_struct[tm][ch] = icu_s;

  //timers are mapped in board.c
  //TODO: map and remap timers based on requested pin
  //vhalPinSetToPeripheral(vpin, PRPH_ICU, ALTERNATE_FN(VP_TOAF(vpin)) | ((ICU_CFG_GET_INPUT(conf->cfg) == ICU_INPUT_PULLUP) ? STM32_PUDR_PULLUP : STM32_PUDR_PULLDOWN));
  //vhalPinSetMode(vpin,((ICU_CFG_GET_INPUT(conf->cfg) == ICU_INPUT_PULLUP) ? PINMODE_INPUT_PULLUP : PINMODE_INPUT_PULLDOWN));
  vhalPinSetToPeripheral(vpin, PRPH_ICU, PIN_PARAMS(PINPRM_MODE_INPUT, PINPRM_CNF_INPUT, 0, 0, 0));

  //icu_s->ithread = vosThCurrent();
  icu_s->port = (GPIO_TypeDef *)PIN_PORT(vpin);
  icu_s->pad = PIN_PAD(vpin);
  icu_s->conf = conf;
  icu_s->conf->time_window = GET_TIME_MICROS(icu_s->conf->time_window);
  icu_s->started = 0;
  icu_s->has_capture = 0;
  if (!conf->endcbk) {
    //if no end callback given, must save current thread
    conf->args = (void *)vosThCurrent();
  }

  TIM_ICInit(timx, ch,(ICU_CFG_GET_TRIGGER(conf->cfg)==1) ? TIM_ICPolarity_Rising : TIM_ICPolarity_Falling);

  timx->DIER |= TIM_IT_Update | (TIM_IT_CC1 << ch); /* Enable the CCx Interrupt sources (TIM_ITConfig in ST stdlib) */
  timx->CR1  |= TIM_CR1_CEN | TIM_CR1_URS;   /* Enable the TIM Counter  (TIM_Cmd in ST stdlib) */

  printf("ARR: %i, PSC: %i\n", timx->ARR, timx->PSC);

  if (timx == TIM1) {
    printf("Enabling CC\n");
    vhalIrqClearPending(TIM1_CC_IRQn);
    vhalIrqEnable(TIM1_CC_IRQn);
  } /*else if (timx.TIMx==TIM8){
    vhalIrqClearPending(TIM8_CC_IRQn);
    vhalIrqEnable(TIM8_CC_IRQn);
  }*/
  vhalIrqClearPending(TIM_IRQ(tm));
  vhalIrqEnable(TIM_IRQ(tm));

  ret = VHAL_OK;

  //blocking
  if (!conf->endcbk) {
    vosSemSignal(icu_sem);
    vosSysLock();
    vosThSuspend();
    vosSysUnlock();
    vosSemWait(icu_sem);
    if (icu_struct[tm][ch]) {
      gc_free(icu_s);
      icu_struct[tm][ch] = NULL;
    }
  }

_ret:
  vosSemSignal(icu_sem);
  return ret;
}


/* ========================================================================
    HTM
   ======================================================================== */

int vhalHtmGetFreeTimer() {
  int prphs = PERIPHERAL_NUM(htm);
  int idx;
  int tmh;
  vosSysLock();
  for (idx = 0; idx < prphs; idx++) {
    tmh = GET_PERIPHERAL_ID(htm, idx);
    if (TIMER_STATUS(tmh) == TIM_STATUS_USABLE) {
      goto end_fn;
    }
  }
  tmh = VHAL_GENERIC_ERROR;
end_fn:
  vosSysUnlock();
  return tmh;
}


void _timSetSimple(uint32_t tm, uint32_t delay, htmFn fn, void *args) {
  TIM_TypeDef *timx = TIM(tm);

  tim_c[tm].fn = fn;
  tim_c[tm].args = args;

  uint32_t prescaler;
  uint32_t period;
  _timeCalcRegisters(delay, &prescaler, &period, 0);
  _set_timer_base(timx, prescaler, period, 0);
  timx->CNT = 0;
  timx->SR = 0;

  vhalIrqClearPending(TIM_IRQ(tm));
  vhalIrqEnable(TIM_IRQ(tm));

  timx->DIER |= TIM_IT_Update; /* Enable the Interrupt sources (TIM_ITConfig in ST stdlib) */
  timx->CR1 |= TIM_CR1_CEN | TIM_CR1_URS;    /* Enable the TIM Counter  (TIM_Cmd in ST stdlib) */
}


int _set_htm_timer(uint32_t tm, uint32_t delay, htmFn fn, void *args, uint32_t status) {
  int ret = VHAL_OK;
  vosSysLock();
  if (TIMER_STATUS(tm) == status && (delay <= 0 || fn == NULL)) {
    _timFree_timer(tm);
    goto end_fn;
  }
  if ( TIMER_STATUS(tm) == TIM_STATUS_USABLE) {
    _timReserveAndInit(tm, status);
    _timSetSimple(tm, delay, fn, args);
    goto end_fn;
  } else ret = VHAL_GENERIC_ERROR;
end_fn:
  vosSysUnlock();
  return ret;
}

int vhalHtmOneShot(uint32_t tm, uint32_t delay, htmFn fn, void *args) {
  return _set_htm_timer(tm, delay, fn, args, TIM_HTM_ONE_SHOT);
}

int vhalHtmRecurrent(uint32_t tm, uint32_t delay, htmFn fn, void *args) {
  return _set_htm_timer(tm, delay, fn, args, TIM_HTM_RECURRENT);
}


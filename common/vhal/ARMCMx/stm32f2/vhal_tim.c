#include "vhal.h"
#include "vhal_gpio.h"
#include "vhal_tim.h"
#include "vhal_irqs.h"

/* 		|TODO|
	
	- capability map for TIMx (6,7,8? TIM_IT_Update event?)
	- parameter check (range for s, ms, us)
	- TIMx check for capability/state

*/

#define VP_TOTIM(X) (PIN_CLASS_DATA0(X) -1)
#define VP_TOCHA(X) (PIN_CLASS_DATA1(X) -1)

/*TODO: driver can be accessed in multiprocess? (lock for get/set status?) */
callback_TypeDef tim_c[TIMNUM];
uint8_t tim_s[TIMNUM]; /* TIMx State */

extern void testdebug(const char *fmt,...);

#define printf(...)

TIMx_TypeDef tim_l[TIMNUM] STORED = {
	{TIM1,  TIM1_UP_TIM10_IRQn, RCC_APBPeriph_TIM1}, /* TODO: define right IRQ not only update ones */
	{TIM2,  TIM2_IRQn, RCC_APBPeriph_TIM2},
	{TIM3,  TIM3_IRQn, RCC_APBPeriph_TIM3},
	{TIM4,  TIM4_IRQn, RCC_APBPeriph_TIM4},
	{TIM5,  TIM5_IRQn, RCC_APBPeriph_TIM5},

	{TIM6,  TIM2_IRQn, RCC_APBPeriph_TIM6},/*       								 */
	{TIM7,  TIM2_IRQn, RCC_APBPeriph_TIM7},/* TODO: find (possible?) IRQ channels */
	{TIM8,  TIM2_IRQn, RCC_APBPeriph_TIM8},/*       								 */

	{TIM9,  TIM1_BRK_TIM9_IRQn, RCC_APBPeriph_TIM9},
	{TIM10, TIM1_UP_TIM10_IRQn, RCC_APBPeriph_TIM10},
	{TIM11, TIM1_TRG_COM_TIM11_IRQn, RCC_APBPeriph_TIM11}
};
uint16_t channel_l[CHANNUM] STORED = {TIM_Channel_1, TIM_Channel_2, TIM_Channel_3, TIM_Channel_4};

uint8_t tim_gpioaf[TIMNUM] STORED = { /*TODO: si possono derivare da qualche parte?*/
  GPIO_AF_TIM1, GPIO_AF_TIM2, GPIO_AF_TIM3, GPIO_AF_TIM4, GPIO_AF_TIM5, 0,
  0, GPIO_AF_TIM8, GPIO_AF_TIM9, GPIO_AF_TIM10, GPIO_AF_TIM11
};

typedef struct {
    uint8_t state;
    VThread  ithread; /* interrupted thread */
    vhalIcuConf* conf; /* ICU conf */
}ICU_state_TypeDef;

ICU_state_TypeDef ICU_state_l[4][CHANNUM];

/* TODO: union, uint_16 invece di 32 per TIMx_TypeDef */

/* TODO: popolate all TIMx IRQ interrupt (global, ...)*/
/*
void vhalIrq_TIM11(void){ _timIrq_wrapper(10); } */ /*TODO: collision! dispatch from irq? TIM1 & TIM10 together?*/ 
void vhalIrq_TIM10(void){ _timIrq_wrapper(9); }
void vhalIrq_TIM9(void) { _timIrq_wrapper(8); }

 /* not working 
void vhalIrq_TIM8(void) { _timIrq_wrapper(7); }
void vhalIrq_TIM7(void) { _timIrq_wrapper(6); } 
void vhalIrq_TIM6(void) { _timIrq_wrapper(5); } 
*/

void vhalIrq_TIM5(void){ _timIrq_wrapper(4); }
void vhalIrq_TIM4(void){ _timIrq_wrapper(3); }
void vhalIrq_TIM3(void){ _timIrq_wrapper(2); }
void vhalIrq_TIM2(void){ _timIrq_wrapper(1); }
void vhalIrq_TIM1(void){ _timIrq_wrapper(0); }


/* PRIVATE fun */

/* from ST */
ITStatus TIM_GetITStatus(TIM_TypeDef* TIMx, uint16_t TIM_IT){
  ITStatus bitstatus = RESET;  
  uint16_t itstatus = 0x0, itenable = 0x0;
  itstatus = TIMx->SR & TIM_IT;
  itenable = TIMx->DIER & TIM_IT;
  if ((itstatus != (uint16_t)RESET) && (itenable != (uint16_t)RESET)) { bitstatus = SET; }
  else { bitstatus = RESET; }
  return bitstatus;
}

void TIM_TimeBaseInit(TIM_TypeDef* TIMx, TIM_TimeBaseInitTypeDef* TIM_TimeBaseInitStruct){
  uint16_t tmpcr1 = 0;

  tmpcr1 = TIMx->CR1;  

  if((TIMx == TIM1) || (TIMx == TIM8)||
     (TIMx == TIM2) || (TIMx == TIM3)||
     (TIMx == TIM4) || (TIMx == TIM5)) 
  {
    /* Select the Counter Mode */
    tmpcr1 &= (uint16_t)(~(TIM_CR1_DIR | TIM_CR1_CMS));
    tmpcr1 |= (uint32_t)TIM_TimeBaseInitStruct->TIM_CounterMode;
  }
 
  if((TIMx != TIM6) && (TIMx != TIM7))
  {
    /* Set the clock division */
    tmpcr1 &=  (uint16_t)(~TIM_CR1_CKD);
    tmpcr1 |= (uint32_t)TIM_TimeBaseInitStruct->TIM_ClockDivision;
  }

  TIMx->CR1 = tmpcr1;

  /* Set the Autoreload value */
  TIMx->ARR = TIM_TimeBaseInitStruct->TIM_Period ;
 
  /* Set the Prescaler value */
  TIMx->PSC = TIM_TimeBaseInitStruct->TIM_Prescaler;
    
  if ((TIMx == TIM1) || (TIMx == TIM8))  
  {
    /* Set the Repetition Counter value */
    TIMx->RCR = TIM_TimeBaseInitStruct->TIM_RepetitionCounter;
  }

  /* Generate an update event to reload the Prescaler 
     and the repetition counter(only for TIM1 and TIM8) value immediately */
  TIMx->EGR = TIM_PSCReloadMode_Immediate;         
}

void NVIC_Init(NVIC_InitTypeDef* NVIC_InitStruct){
  uint8_t tmppriority = 0x00, tmppre = 0x00, tmpsub = 0x0F;
  
    
  if (NVIC_InitStruct->NVIC_IRQChannelCmd != DISABLE)
  {
    tmppriority = (0x700 - ((SCB->AIRCR) & (uint32_t)0x700))>> 0x08;
    tmppre = (0x4 - tmppriority);
    tmpsub = tmpsub >> tmppriority;

    tmppriority = NVIC_InitStruct->NVIC_IRQChannelPreemptionPriority << tmppre;
    tmppriority |=  (uint8_t)(NVIC_InitStruct->NVIC_IRQChannelSubPriority & tmpsub);
        
    tmppriority = tmppriority << 0x04;
        
    NVIC->IP[NVIC_InitStruct->NVIC_IRQChannel] = tmppriority;
    
    NVIC->ISER[NVIC_InitStruct->NVIC_IRQChannel >> 0x05] =
      (uint32_t)0x01 << (NVIC_InitStruct->NVIC_IRQChannel & (uint8_t)0x1F);
  }
  else
  {
    NVIC->ICER[NVIC_InitStruct->NVIC_IRQChannel >> 0x05] =
      (uint32_t)0x01 << (NVIC_InitStruct->NVIC_IRQChannel & (uint8_t)0x1F);
  }
}

void GPIO_PinAFConfig(GPIO_TypeDef* GPIOx, uint16_t GPIO_PinSource, uint8_t GPIO_AF)
{
  uint32_t temp = 0x00;
  uint32_t temp_2 = 0x00;
  
  temp = ((uint32_t)(GPIO_AF) << ((uint32_t)((uint32_t)GPIO_PinSource & (uint32_t)0x07) * 4)) ;
  GPIOx->AFR[GPIO_PinSource >> 0x03] &= ~((uint32_t)0xF << ((uint32_t)((uint32_t)GPIO_PinSource & (uint32_t)0x07) * 4)) ;
  temp_2 = GPIOx->AFR[GPIO_PinSource >> 0x03] | temp;
  GPIOx->AFR[GPIO_PinSource >> 0x03] = temp_2;
}

void GPIO_Init(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_InitStruct){
  uint32_t pinpos = 0x00, pos = 0x00 , currentpin = 0x00;

  /* ------------------------- Configure the port pins ---------------- */
  /*-- GPIO Mode Configuration --*/
  for (pinpos = 0x00; pinpos < 0x10; pinpos++)
  {
    pos = ((uint32_t)0x01) << pinpos;
    /* Get the port pins position */
    currentpin = (GPIO_InitStruct->GPIO_Pin) & pos;

    if (currentpin == pos)
    {
      GPIOx->MODER  &= ~(GPIO_MODER_MODER0 << (pinpos * 2));
      GPIOx->MODER |= (((uint32_t)GPIO_InitStruct->GPIO_Mode) << (pinpos * 2));

      if ((GPIO_InitStruct->GPIO_Mode == GPIO_Mode_OUT) || (GPIO_InitStruct->GPIO_Mode == GPIO_Mode_AF))
      {

        /* Speed mode configuration */
        GPIOx->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR0 << (pinpos * 2));
        GPIOx->OSPEEDR |= ((uint32_t)(GPIO_InitStruct->GPIO_Speed) << (pinpos * 2));

        /* Output mode configuration*/
        GPIOx->OTYPER  &= ~((GPIO_OTYPER_OT_0) << ((uint16_t)pinpos)) ;
        GPIOx->OTYPER |= (uint16_t)(((uint16_t)GPIO_InitStruct->GPIO_OType) << ((uint16_t)pinpos));
      }

      /* Pull-up Pull down resistor configuration*/
      GPIOx->PUPDR &= ~(GPIO_PUPDR_PUPDR0 << ((uint16_t)pinpos * 2));
      GPIOx->PUPDR |= (((uint32_t)GPIO_InitStruct->GPIO_PuPd) << (pinpos * 2));
    }
  }
}

void TIM_ICInit(TIM_TypeDef* TIMx, TIM_ICInitTypeDef* TIM_ICInitStruct){
  if (TIM_ICInitStruct->TIM_Channel == TIM_Channel_1)
  {
    uint16_t tmpccmr1 = 0, tmpccer = 0;
    /* Disable the Channel 1: Reset the CC1E Bit */
    TIMx->CCER &= (uint16_t)~TIM_CCER_CC1E;
    tmpccmr1 = TIMx->CCMR1;
    tmpccer = TIMx->CCER;
    /* Select the Input and set the filter */
    tmpccmr1 &= ((uint16_t)~TIM_CCMR1_CC1S) & ((uint16_t)~TIM_CCMR1_IC1F);
    tmpccmr1 |= (uint16_t)(TIM_ICInitStruct->TIM_ICSelection | (uint16_t)(TIM_ICInitStruct->TIM_ICFilter << (uint16_t)4));
    /* Select the Polarity and set the CC1E Bit */
    tmpccer &= (uint16_t)~(TIM_CCER_CC1P | TIM_CCER_CC1NP);
    tmpccer |= (uint16_t)(TIM_ICInitStruct->TIM_ICPolarity | (uint16_t)TIM_CCER_CC1E);
    /* Write to TIMx CCMR1 and CCER registers */
    TIMx->CCMR1 = tmpccmr1;
    TIMx->CCER = tmpccer;

    /* Reset the IC1PSC Bits */
    TIMx->CCMR1 &= (uint16_t)~TIM_CCMR1_IC1PSC;
    /* Set the IC1PSC value */
    TIMx->CCMR1 |= TIM_ICInitStruct->TIM_ICPrescaler;

  }
  else if (TIM_ICInitStruct->TIM_Channel == TIM_Channel_2)
  {
    uint16_t tmpccmr1 = 0, tmpccer = 0, tmp = 0;

    /* Disable the Channel 2: Reset the CC2E Bit */
    TIMx->CCER &= (uint16_t)~TIM_CCER_CC2E;
    tmpccmr1 = TIMx->CCMR1;
    tmpccer = TIMx->CCER;
    tmp = (uint16_t)(TIM_ICInitStruct->TIM_ICPolarity << 4);
    /* Select the Input and set the filter */
    tmpccmr1 &= ((uint16_t)~TIM_CCMR1_CC2S) & ((uint16_t)~TIM_CCMR1_IC2F);
    tmpccmr1 |= (uint16_t)(TIM_ICInitStruct->TIM_ICFilter << 12);
    tmpccmr1 |= (uint16_t)(TIM_ICInitStruct->TIM_ICSelection << 8);
    /* Select the Polarity and set the CC2E Bit */
    tmpccer &= (uint16_t)~(TIM_CCER_CC2P | TIM_CCER_CC2NP);
    tmpccer |=  (uint16_t)(tmp | (uint16_t)TIM_CCER_CC2E);
    /* Write to TIMx CCMR1 and CCER registers */
    TIMx->CCMR1 = tmpccmr1 ;
    TIMx->CCER = tmpccer;

    /* Reset the IC2PSC Bits */
    TIMx->CCMR1 &= (uint16_t)~TIM_CCMR1_IC2PSC;
    /* Set the IC2PSC value */
    TIMx->CCMR1 |= (uint16_t)(TIM_ICInitStruct->TIM_ICPrescaler << 8);
  }
  else if (TIM_ICInitStruct->TIM_Channel == TIM_Channel_3)
  {
    uint16_t tmpccmr2 = 0, tmpccer = 0, tmp = 0;

    /* Disable the Channel 3: Reset the CC3E Bit */
    TIMx->CCER &= (uint16_t)~TIM_CCER_CC3E;
    tmpccmr2 = TIMx->CCMR2;
    tmpccer = TIMx->CCER;
    tmp = (uint16_t)(TIM_ICInitStruct->TIM_ICPolarity << 8);
    /* Select the Input and set the filter */
    tmpccmr2 &= ((uint16_t)~TIM_CCMR1_CC1S) & ((uint16_t)~TIM_CCMR2_IC3F);
    tmpccmr2 |= (uint16_t)(TIM_ICInitStruct->TIM_ICSelection | (uint16_t)(TIM_ICInitStruct->TIM_ICFilter << (uint16_t)4));
    /* Select the Polarity and set the CC3E Bit */
    tmpccer &= (uint16_t)~(TIM_CCER_CC3P | TIM_CCER_CC3NP);
    tmpccer |= (uint16_t)(tmp | (uint16_t)TIM_CCER_CC3E);
    /* Write to TIMx CCMR2 and CCER registers */
    TIMx->CCMR2 = tmpccmr2;
    TIMx->CCER = tmpccer;

    /* Reset the IC3PSC Bits */
    TIMx->CCMR2 &= (uint16_t)~TIM_CCMR2_IC3PSC;
    /* Set the IC3PSC value */
    TIMx->CCMR2 |= TIM_ICInitStruct->TIM_ICPrescaler;
  }
  else
  {
    uint16_t tmpccmr2 = 0, tmpccer = 0, tmp = 0;

    /* Disable the Channel 4: Reset the CC4E Bit */
    TIMx->CCER &= (uint16_t)~TIM_CCER_CC4E;
    tmpccmr2 = TIMx->CCMR2;
    tmpccer = TIMx->CCER;
    tmp = (uint16_t)(TIM_ICInitStruct->TIM_ICPolarity << 12);
    /* Select the Input and set the filter */
    tmpccmr2 &= ((uint16_t)~TIM_CCMR1_CC2S) & ((uint16_t)~TIM_CCMR1_IC2F);
    tmpccmr2 |= (uint16_t)(TIM_ICInitStruct->TIM_ICSelection << 8);
    tmpccmr2 |= (uint16_t)(TIM_ICInitStruct->TIM_ICFilter << 12);
    /* Select the Polarity and set the CC4E Bit */
    tmpccer &= (uint16_t)~(TIM_CCER_CC4P | TIM_CCER_CC4NP);
    tmpccer |= (uint16_t)(tmp | (uint16_t)TIM_CCER_CC4E);
    /* Write to TIMx CCMR2 and CCER registers */
    TIMx->CCMR2 = tmpccmr2;
    TIMx->CCER = tmpccer ;

    /* Reset the IC4PSC Bits */
    TIMx->CCMR2 &= (uint16_t)~TIM_CCMR2_IC4PSC;
    /* Set the IC4PSC value */
    TIMx->CCMR2 |= (uint16_t)(TIM_ICInitStruct->TIM_ICPrescaler << 8);
  }
}

void TIM_OC1PolarityConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCPolarity){
  uint16_t tmpccer = 0;

  tmpccer = TIMx->CCER;
  /* Set or Reset the CC1P Bit */
  tmpccer &= (uint16_t)(~TIM_CCER_CC1P);
  tmpccer |= TIM_OCPolarity;
  /* Write to TIMx CCER register */
  TIMx->CCER = tmpccer;
}

/* END from ST*/

void _timOCxPreloadConfig(uint8_t chan, TIM_TypeDef* TIMx, uint16_t TIM_OCPreload){
  uint16_t tmpccmr = 0;

  switch (chan) {
      case 0: 
          tmpccmr = TIMx->CCMR1;
          tmpccmr &= (uint16_t)(~TIM_CCMR1_OC1PE);
          tmpccmr |= TIM_OCPreload;
          TIMx->CCMR1 = tmpccmr;

          break;
      case 1: 
          tmpccmr = TIMx->CCMR1;
          tmpccmr &= (uint16_t)(~TIM_CCMR1_OC2PE);
          tmpccmr |= (uint16_t)(TIM_OCPreload << 8);
          TIMx->CCMR1 = tmpccmr;
          break;
      case 2: 
          tmpccmr = TIMx->CCMR2;
          tmpccmr &= (uint16_t)(~TIM_CCMR2_OC3PE);
          tmpccmr |= TIM_OCPreload;
          TIMx->CCMR2 = tmpccmr;
          break;
      case 3: 
          tmpccmr = TIMx->CCMR2;
          tmpccmr &= (uint16_t)(~TIM_CCMR2_OC4PE);
          tmpccmr |= (uint16_t)(TIM_OCPreload << 8);
          TIMx->CCMR2 = tmpccmr;
          break;          
  }
}

void _timOCxInit(uint8_t chan, TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct){
  uint16_t tmpccmrx = 0, tmpccer = 0, tmpcr2 = 0;
  switch (chan) {
      case 0: 
          TIMx->CCER &= (uint16_t)~TIM_CCER_CC1E;
          tmpccer = TIMx->CCER;
          tmpcr2 =  TIMx->CR2;
          tmpccmrx = TIMx->CCMR1;
          tmpccmrx &= (uint16_t)~TIM_CCMR1_OC1M;
          tmpccmrx &= (uint16_t)~TIM_CCMR1_CC1S;
          tmpccmrx |= TIM_OCInitStruct->TIM_OCMode;
          tmpccer &= (uint16_t)~TIM_CCER_CC1P;
          tmpccer |= TIM_OCInitStruct->TIM_OCPolarity;
          tmpccer |= TIM_OCInitStruct->TIM_OutputState;
          if((TIMx == TIM1) || (TIMx == TIM8)) {
            tmpccer &= (uint16_t)~TIM_CCER_CC1NP;
            tmpccer |= TIM_OCInitStruct->TIM_OCNPolarity;
            tmpccer &= (uint16_t)~TIM_CCER_CC1NE;
            tmpccer |= TIM_OCInitStruct->TIM_OutputNState;
            tmpcr2 &= (uint16_t)~TIM_CR2_OIS1;
            tmpcr2 &= (uint16_t)~TIM_CR2_OIS1N;
            tmpcr2 |= TIM_OCInitStruct->TIM_OCIdleState;
            tmpcr2 |= TIM_OCInitStruct->TIM_OCNIdleState;
          }
          TIMx->CR2 = tmpcr2;
          TIMx->CCMR1 = tmpccmrx;
          TIMx->CCR1 = TIM_OCInitStruct->TIM_Pulse;
          break;
      case 1: 
          TIMx->CCER &= (uint16_t)~TIM_CCER_CC2E;
          tmpccer = TIMx->CCER;
          tmpcr2 =  TIMx->CR2;
          tmpccmrx = TIMx->CCMR1;
          tmpccmrx &= (uint16_t)~TIM_CCMR1_OC2M;
          tmpccmrx &= (uint16_t)~TIM_CCMR1_CC2S;
          tmpccmrx |= (uint16_t)(TIM_OCInitStruct->TIM_OCMode << 8);
          tmpccer &= (uint16_t)~TIM_CCER_CC2P;
          tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OCPolarity << 4);
          tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OutputState << 4);
          if((TIMx == TIM1) || (TIMx == TIM8)) {
            tmpccer &= (uint16_t)~TIM_CCER_CC2NP;
            tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OCNPolarity << 4);
            tmpccer &= (uint16_t)~TIM_CCER_CC2NE;
            tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OutputNState << 4);
            tmpcr2 &= (uint16_t)~TIM_CR2_OIS2;
            tmpcr2 &= (uint16_t)~TIM_CR2_OIS2N;
            tmpcr2 |= (uint16_t)(TIM_OCInitStruct->TIM_OCIdleState << 2);
            tmpcr2 |= (uint16_t)(TIM_OCInitStruct->TIM_OCNIdleState << 2);
          }
          TIMx->CR2 = tmpcr2;
          TIMx->CCMR1 = tmpccmrx;
          TIMx->CCR2 = TIM_OCInitStruct->TIM_Pulse;
          break;
      case 2: 
          TIMx->CCER &= (uint16_t)~TIM_CCER_CC3E;
          tmpccer = TIMx->CCER;
          tmpcr2 =  TIMx->CR2;
          tmpccmrx = TIMx->CCMR2;
          tmpccmrx &= (uint16_t)~TIM_CCMR2_OC3M;
          tmpccmrx &= (uint16_t)~TIM_CCMR2_CC3S;
          tmpccmrx |= TIM_OCInitStruct->TIM_OCMode;
          tmpccer &= (uint16_t)~TIM_CCER_CC3P;
          tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OCPolarity << 8);
          tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OutputState << 8);
          if((TIMx == TIM1) || (TIMx == TIM8)) {
            tmpccer &= (uint16_t)~TIM_CCER_CC3NP;
            tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OCNPolarity << 8);
            tmpccer &= (uint16_t)~TIM_CCER_CC3NE;
            tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OutputNState << 8);
            tmpcr2 &= (uint16_t)~TIM_CR2_OIS3;
            tmpcr2 &= (uint16_t)~TIM_CR2_OIS3N;
            tmpcr2 |= (uint16_t)(TIM_OCInitStruct->TIM_OCIdleState << 4);
            tmpcr2 |= (uint16_t)(TIM_OCInitStruct->TIM_OCNIdleState << 4);
          }
          TIMx->CR2 = tmpcr2;
          TIMx->CCMR2 = tmpccmrx;
          TIMx->CCR3 = TIM_OCInitStruct->TIM_Pulse;
          break;
      case 3: 
          TIMx->CCER &= (uint16_t)~TIM_CCER_CC4E;
          tmpccer = TIMx->CCER;
          tmpcr2 =  TIMx->CR2;
          tmpccmrx = TIMx->CCMR2;
          tmpccmrx &= (uint16_t)~TIM_CCMR2_OC4M;
          tmpccmrx &= (uint16_t)~TIM_CCMR2_CC4S;
          tmpccmrx |= (uint16_t)(TIM_OCInitStruct->TIM_OCMode << 8);
          tmpccer &= (uint16_t)~TIM_CCER_CC4P;
          tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OCPolarity << 12);
          tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OutputState << 12);
          if((TIMx == TIM1) || (TIMx == TIM8)) {
            tmpcr2 &=(uint16_t) ~TIM_CR2_OIS4;
            tmpcr2 |= (uint16_t)(TIM_OCInitStruct->TIM_OCIdleState << 6);
          }
          TIMx->CR2 = tmpcr2;
          TIMx->CCMR2 = tmpccmrx;
          TIMx->CCR4 = TIM_OCInitStruct->TIM_Pulse;
          break;  
  }
  TIMx->CCER = tmpccer;
  TIMx->EGR |= ((uint8_t)0x01);
}

struct PWM_State {
    uint8_t  state;
    uint16_t rise;
    uint16_t fall;
}Inputs = { 0, };

uint16_t buffero[256];
volatile uint8_t fine = 0;
volatile uint8_t icuc = 0;
uint16_t lena = 0;


void testtest(){
//   while(1){  
//        vosSysLock();
//        if(fine){   
//         printf("len: %i, icuc: %i\n", lena, icuc);
//         uint16_t i;
//         for(i=0; i<lena; i++)  printf("b[%i]: %i\n",i, buffero[i]);//Print the total high level time
//         vosSysUnlock();
//         return;
//       }
//     vosSysUnlock();
//     }
  printf("len: %i, icuc: %i\n", lena, icuc);
  uint16_t i;
  for(i=0; i<lena; i++)  printf("b[%i]: %i\n",i, buffero[i]);//Print the total high level time
}

volatile uint8_t update_event = 1;
volatile uint8_t first = 1;


void _timICUResumeth(VThread t){
  vosSysLockIsr();
  vosThResume(t);
  vosSysUnlockIsr();
}

void _timICUIrq(uint8_t tm){ 
  static uint32_t Current;
  TIM_TypeDef* TIMx = tim_l[tm].TIMx;

  if (TIM_GetITStatus(TIMx, TIM_IT_CC1) != RESET) {
        Current = TIMx->CCR1;
        update_event = 0;
        icuc++;
        tim_l[tm].TIMx->SR = (uint16_t)~TIM_IT_CC1;
        if (Inputs.state == 0) {
            Inputs.rise = Current;
            if(first){ 
              Inputs.fall = 0; 
              first = 0;
            }else {
              buffero[lena] = Inputs.rise - Inputs.fall;
              lena++; 
            }
            Inputs.state = 1;
        } else {
            Inputs.fall = Current;
            buffero[lena] = Inputs.fall - Inputs.rise;
            lena++;
            Inputs.state = 0;
        }
        if (ICU_state_l[tm][0].conf->bufsize == lena){
              _timICUResumeth(ICU_state_l[tm][0].ithread);
        }
    }
  if (TIM_GetITStatus(tim_l[tm].TIMx, TIM_IT_Update) != RESET) {
      tim_l[tm].TIMx->SR = (uint16_t)~TIM_IT_Update;
      _timICUResumeth(ICU_state_l[tm][0].ithread);
      //fine = 1;
  }
}

void _timFree_timer(uint8_t tm, uint8_t from_interrupt){
  tim_l[tm].TIMx->SR = (uint16_t)~TIM_IT_Update; 
  tim_l[tm].TIMx->DIER &= (uint16_t)~TIM_IT_Update;
  tim_l[tm].TIMx->CR1 &= (uint16_t)~TIM_CR1_CEN;
  if(from_interrupt){
    vosSysLockIsr();
    tim_s[tm] = TIM_STATUS_USABLE;
    vosSysUnlockIsr();
  }
  else{
    vosSysLock();
    tim_s[tm] = TIM_STATUS_USABLE;
    vosSysUnlock();
  }
}

void _timIrq_wrapper(uint8_t tm){
  if(tim_s[tm] == TIM_STATUS_ICU) {

    _timICUIrq(tm);
  }
  else {
    if(tim_s[tm] == TIM_STATUS_ONESHOT ||tim_s[tm] == TIM_STATUS_RECURENT){
  	  if (TIM_GetITStatus(tim_l[tm].TIMx, TIM_IT_Update) != RESET){
          tim_l[tm].TIMx->SR = (uint16_t)~TIM_IT_Update;  /* Clear the IT pending Bit */
          if(*tim_c[tm].fn){ (*tim_c[tm].fn)(tm, tim_c[tm].args); } /* execute callback */
      }
      if( tim_s[tm] == TIM_STATUS_ONESHOT){ /* free TIMx */
      	    _timFree_timer(tm, TRUE);
      }
    }
}
}

void _timReserveAndInit(uint32_t tm, uint8_t status) {
	/* Enables the Low Speed APB (APB1/2) peripheral clock. */
    if(IS_RCC_APB1_PERIPH(tim_l[tm].RCC_APBPeriph)){
    	RCC->APB1ENR |= tim_l[tm].RCC_APBPeriph;
    }
    if(IS_RCC_APB2_PERIPH(tim_l[tm].RCC_APBPeriph) ){ /* BUG? TIM1 APBPeriph == TIM2 why?? (or TIM3 & TIM8)*/
    	RCC->APB2ENR |= tim_l[tm].RCC_APBPeriph;
    }
	tim_l[tm].TIMx->SR = (uint16_t)~TIM_IT_Update;
  tim_s[tm] = status;
}

uint8_t _timeCalculatePeriod(uint32_t delay, uint32_t* TIM_Prescaler, uint32_t* TIM_Period){
  uint32_t ctime = GET_TIME_VALUE(delay);
  uint32_t t_unit = GET_TIME_UNIT(delay);
  if(t_unit == MICROS && ctime> 65534){
    ctime = ctime/1000;
    t_unit = MILLIS;
  }

  if(t_unit == MILLIS && ctime> 6552){
    ctime = ctime/1000;
    t_unit = SECONDS;
  }
  /* TODO: costante dipende da _system_frequency (7 etc)*/

  switch (t_unit) {
      case MICROS: /* range 0 - 65535 */
          *TIM_Prescaler = (uint16_t) ((_system_frequency / 1000000) - 1); /* To 0.001 ms */
          *TIM_Period = (uint16_t) ((ctime) - 1);
          break;
      case MILLIS: /* range 0 - 6553 */
          *TIM_Prescaler = (uint16_t) ((_system_frequency / 10000)*7 - 1); /* To 0.7 ms */
          *TIM_Period = (uint16_t) ((ctime * 1000/700) - 1);
          break;
      case SECONDS: /* range 0 - 32 */
          *TIM_Prescaler = (uint16_t) (((_system_frequency / 10000)*5) - 1); /* To 0.5 ms */
          *TIM_Period = (uint16_t) ((ctime * 2000) - 1);
          break;
  }
  return 0;
}

void _timeBaseInit(TIM_TypeDef* TIMx, uint32_t delay, uint16_t countermode, uint16_t clockdivision, uint8_t repetitioncounter){
  uint16_t prescaler;
  uint16_t period;
  _timeCalculatePeriod(delay, (uint32_t*)&prescaler, (uint32_t*)&period);

  TIM_TimeBaseInitTypeDef timerInitStructure;
  timerInitStructure.TIM_Prescaler = prescaler;
  timerInitStructure.TIM_Period = period;
  timerInitStructure.TIM_ClockDivision = clockdivision;
  timerInitStructure.TIM_CounterMode = countermode;
  timerInitStructure.TIM_RepetitionCounter = repetitioncounter;
  TIM_TimeBaseInit(TIMx, &timerInitStructure);
  TIMx->CNT = 0;
  TIMx->SR = 0;
}

void _timSetSimple(uint32_t tm, uint32_t delay, htmFn fn, void *args){
  TIMx_TypeDef timx = tim_l[tm];

  tim_c[tm].fn = fn;
  tim_c[tm].args = args;

  _timeBaseInit(timx.TIMx, delay, TIM_CounterMode_Up, 0, 0);

  NVIC_InitTypeDef nvicStructure;
  nvicStructure.NVIC_IRQChannel = timx.IRQ;
  nvicStructure.NVIC_IRQChannelPreemptionPriority = 0;
  nvicStructure.NVIC_IRQChannelSubPriority = 1;
  nvicStructure.NVIC_IRQChannelCmd = ENABLE;
  vhalIrqClearPending(timx.IRQ);
  vhalIrqInit(&nvicStructure);

  timx.TIMx->DIER |= TIM_IT_Update; /* Enable the Interrupt sources (TIM_ITConfig in ST stdlib) */
  timx.TIMx->CR1 |= TIM_CR1_CEN | TIM_CR1_URS;    /* Enable the TIM Counter  (TIM_Cmd in ST stdlib) */
}

/* END PRIVATE fun*/


int vhalInitTIM(void *data) {
    (void)data;
    uint8_t i;
    for(i=0; i < TIMNUM; i++) { tim_s[i] = TIM_STATUS_USABLE; }
    return 0;
} 


int vhalPwmStart(int vpin, uint32_t period, uint32_t pulse) {
	if(period == 0) { 	
		 _timFree_timer(VP_TOTIM(vpin), FALSE);
		return 0; 
	}

	TIMx_TypeDef timx = tim_l[VP_TOTIM(vpin)]; /* leggo il timer dal vpin (-1 per indice)*/

	_timReserveAndInit(VP_TOTIM(vpin), TIM_STATUS_PWM);

	vhalPinSetToPeripheral(vpin,PRPH_PWM,ALTERNATE_FN(tim_gpioaf[VP_TOTIM(vpin)])|STM32_PUDR_NOPULL|STM32_OSPEED_HIGHEST|STM32_OTYPE_PUSHPULL);

	/*
	GPIOA PIN_PORT(vpin)
	GPIO_PinSource5 PIN_PAD(vpin)
	GPIO_Pin_5 1<<PIN_PAD

	*/

	TIM_OCInitTypeDef TIM_OCInitStructure;
	uint32_t _tmp;
	uint32_t tim_period=0;
	uint32_t pwm_period=0;

	_timeBaseInit(timx.TIMx, period, TIM_CounterMode_Up, TIM_CKD_DIV1, 1);

	/*
	timx.TIMx->CR1 &= ~TIM_CR1_OPM;
	timx.TIMx->CR2 &= ~TIM_CR2_MMS_MASK;
	timx.TIMx->CR2 |= TIM_CR2_MMS_UPDATE;
	*/

	_timeCalculatePeriod(period, &_tmp, &tim_period);
	_timeCalculatePeriod(pulse, &_tmp, &pwm_period);

	printf("%i - %i, timer: %i, channel: %i\n",tim_period,pwm_period, PIN_CLASS_DATA0(vpin), PIN_CLASS_DATA1(vpin));

	/* Set the ARR Preload Bit */
	timx.TIMx->CR1 |= TIM_CR1_ARPE;

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = ((tim_period*pwm_period)/tim_period)-1; //TIME_U(5, SECONDS),
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;

	_timOCxInit(VP_TOCHA(vpin), timx.TIMx, &TIM_OCInitStructure);
	_timOCxPreloadConfig(VP_TOCHA(vpin), timx.TIMx, TIM_OCPreload_Enable);

	timx.TIMx->BDTR |= TIM_BDTR_MOE;  /* Enable the TIM Main Output (TIM_CtrlPWMOutputs in ST stdlib)*/
	timx.TIMx->CR1 |= TIM_CR1_CEN;    /* Enable the TIM Counter  (TIM_Cmd in ST stdlib) */

	return 0;
}

int vhalIcuStart(int vpin, vhalIcuConf *conf) {
	if(conf == NULL || conf->buffer == NULL || conf->bufsize == 0) { 	
		 _timFree_timer(VP_TOTIM(vpin), FALSE);
		return 0; 
	}

	TIM_ICInitTypeDef TIM_ICInitStructure;
	TIMx_TypeDef timx = tim_l[VP_TOTIM(vpin)]; /* leggo il timer dal vpin (-1 per indice)*/
	printf("ICU timer: %i, channel: %i (%i,%i)\n", PIN_CLASS_DATA0(vpin), PIN_CLASS_DATA1(vpin),VP_TOTIM(vpin), VP_TOCHA(vpin));

	if(ICU_state_l[VP_TOTIM(vpin)][VP_TOCHA(vpin)].state == ICU_STATUS_USABLE){
		printf("ICU SET\n");
		/* TODO: usare un solo state: di timer non di ICU */
		vosSysLock();
		ICU_state_l[VP_TOTIM(vpin)][VP_TOCHA(vpin)].state = ICU_STATUS_RUNNING;

		ICU_state_l[VP_TOTIM(vpin)][VP_TOCHA(vpin)].ithread = vosThCurrent();
		//vosThSuspend();
		vosSysUnlock();

		ICU_state_l[VP_TOTIM(vpin)][VP_TOCHA(vpin)].conf = conf;

		_timReserveAndInit(VP_TOTIM(vpin), TIM_STATUS_ICU);

		vhalPinSetToPeripheral(vpin,PRPH_PWM,ALTERNATE_FN(tim_gpioaf[VP_TOTIM(vpin)])|STM32_MODE_INPUT|STM32_PUDR_PULLDOWN);

		_timeBaseInit(timx.TIMx, TIME_U(6, SECONDS), TIM_CounterMode_Up, 0,0);

		/* Initialize the TIMx input capture parameters */
		TIM_ICInitStructure.TIM_Channel = channel_l[VP_TOCHA(vpin)]; /*CC1S=01     Select the input IC1 is mapped to TI1*/    
		TIM_ICInitStructure.TIM_ICPolarity =  TIM_ICPolarity_BothEdge; /*Rising along the capture*/
		TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI; /*Mapping to the TI1*/
		TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1; /*Configure input frequency, regardless of the frequency*/
		TIM_ICInitStructure.TIM_ICFilter = 0; /*The IC1F=0000 configuration input filter without filter*/
		TIM_ICInit(timx.TIMx,&TIM_ICInitStructure);

		NVIC_InitTypeDef nvicStructure;
		nvicStructure.NVIC_IRQChannel = timx.IRQ;
		nvicStructure.NVIC_IRQChannelPreemptionPriority = 2;
		nvicStructure.NVIC_IRQChannelSubPriority = 0;
		nvicStructure.NVIC_IRQChannelCmd = ENABLE;
		vhalIrqInit(&nvicStructure);

		timx.TIMx->DIER |= TIM_IT_Update|TIM_IT_CC1; /* Enable the CC1 Interrupt sources (TIM_ITConfig in ST stdlib) */
		timx.TIMx->CR1 |= TIM_CR1_CEN;    /* Enable the TIM Counter  (TIM_Cmd in ST stdlib) */

		return 0;
	}
	return -1;
}

int vhalHtmGetFreeTimer(){
	int idx;
	vosSysLock();
	for(idx = 10; idx>=0; idx--){
		if(tim_s[idx] == TIM_STATUS_USABLE ) { return idx; }
	}
	vosSysUnlock();

	return 0;
}

int vhalHtmOneShot(uint32_t tm, uint32_t delay, htmFn fn, void *args){
	if(delay <=0 || fn == NULL) { 	
		 _timFree_timer(tm, FALSE);
		return 0; 
	}
	_timReserveAndInit(tm, TIM_STATUS_ONESHOT);
	_timSetSimple(tm, delay, fn, args);
	return 0;
}


int vhalHtmRecurrent(uint32_t tm, uint32_t delay, htmFn fn, void *args){
	if(delay <=0 || fn == NULL) { 	
    _timFree_timer(tm, FALSE);
		return 0; 
	}
	_timReserveAndInit(tm, TIM_STATUS_RECURENT);
	_timSetSimple(tm, delay, fn, args);
	return 0;
}





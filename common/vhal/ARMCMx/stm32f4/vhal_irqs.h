#ifndef __VHAL_VECTORS__
#define __VHAL_VECTORS__



/* ========================================================================
    ISR VECTORS
   ======================================================================== */

/*
	Refer to "Interrupts and events" section of docs/RM_*
*/


/*
	Irq names are taken from Chibi. When other RTOS will be supported,
	either we #ifdef on the RTOS or we #define the irq names in vosal.h in such a way
	that they are os independent.

*/
#ifdef RTOS_chibi2


extern void nvicEnableVector(uint32_t n, uint32_t prio);
extern void nvicDisableVector(uint32_t n);

#define vhalIrqEnablePrio(irqn,prio) nvicEnableVector(irqn,prio)
#define vhalIrqDisable(irqn) nvicDisableVector(irqn)

#define vhalIrqEnable(irqn) vhalIrqEnablePrio(irqn,6)
#define vhalIrqClearPending(IRQn) NVIC->ICPR[((uint32_t)(IRQn) >> 5)] = (1 << ((uint32_t)(IRQn) & 0x1F))

#define vhalIrq_WWDG         Vector40
#define vhalIrq_EXT16_PVD    Vector44
#define vhalIrq_EXT21_TAMP   Vector48
#define vhalIrq_EXT2_RTC     Vector4C
#define vhalIrq_FLASH Vector50
#define vhalIrq_RCC Vector54
#define vhalIrq_EXT0 Vector58
#define vhalIrq_EXT1 Vector5C
#define vhalIrq_EXT2 Vector60
#define vhalIrq_EXT3 Vector64
#define vhalIrq_EXT4 Vector68
#define vhalIrq_DMA1_S0 Vector6C
#define vhalIrq_DMA1_S1 Vector70
#define vhalIrq_DMA1_S2 Vector74
#define vhalIrq_DMA1_S3 Vector78
#define vhalIrq_DMA1_S4 Vector7C
#define vhalIrq_DMA1_S5 Vector80
#define vhalIrq_DMA1_S6 Vector84
#define vhalIrq_ADC1 Vector88
#define vhalIrq_CAN1_TX Vector8C
#define vhalIrq_CAN1_RX0 Vector90
#define vhalIrq_CAN1_RX1 Vector94
#define vhalIrq_CAN1_SCE Vector98
#define vhalIrq_EXT9_5 Vector9C
#define vhalIrq_TIM1_BRK_TIM9 VectorA0
#define vhalIrq_TIM1_UPD_TIM10 VectorA4
#define vhalIrq_TIM1_TRG_COM_TIM11 VectorA8
#define vhalIrq_TIM1_CC VectorAC
#define vhalIrq_TIM2 VectorB0
#define vhalIrq_TIM3 VectorB4
#define vhalIrq_TIM4 VectorB8
#define vhalIrq_I2C1_EV VectorBC
#define vhalIrq_I2C1_ER VectorC0
#define vhalIrq_I2C2_EV VectorC4
#define vhalIrq_I2C2_ER VectorC8
#define vhalIrq_SPI1 VectorCC
#define vhalIrq_SPI2 VectorD0
#define vhalIrq_USART1 VectorD4
#define vhalIrq_USART2 VectorD8
#define vhalIrq_USART3 VectorDC
#define vhalIrq_EXT15_10 VectorE0
#define vhalIrq_RTC_ALARM VectorE4
#define vhalIrq_OTG_WKUP VectorE8
#define vhalIrq_TIM8_BRK_TIM12 VectorEC
#define vhalIrq_TIM8_UP_TIM13 VectorF0
#define vhalIrq_TIM8_TRG_COM_TIM14 VectorF4
#define vhalIrq_TIM8_CC VectorF8
#define vhalIrq_DMA1_S7 VectorFC
#define vhalIrq_FSMC Vector100
#define vhalIrq_SDIO Vector104
#define vhalIrq_TIM5 Vector108
#define vhalIrq_SPI3 Vector10C
#define vhalIrq_UART4 Vector110
#define vhalIrq_UART5 Vector114
#define vhalIrq_TIM6_DAC Vector118
#define vhalIrq_TIM7 Vector11C
#define vhalIrq_DMA2_S0 Vector120
#define vhalIrq_DMA2_S1 Vector124
#define vhalIrq_DMA2_S2 Vector128
#define vhalIrq_DMA2_S3 Vector12C
#define vhalIrq_DMA2_S4 Vector130
#define vhalIrq_ETH Vector134
#define vhalIrq_ETH_WKUP Vector138
#define vhalIrq_CAN2_TX Vector13C
#define vhalIrq_CAN2_RX0 Vector140
#define vhalIrq_CAN2_RX1 Vector144
#define vhalIrq_CAN2_SCE Vector148
#define vhalIrq_OTG Vector14C
#define vhalIrq_DMA2_S5 Vector150
#define vhalIrq_DMA2_S6 Vector154
#define vhalIrq_DMA2_S7 Vector158
#define vhalIrq_USART6 Vector15C
#define vhalIrq_I2C3_EV Vector160
#define vhalIrq_I2C3_ER Vector164
#define vhalIrq_OTG_HS_EP1_OUT Vector168
#define vhalIrq_OTG_HS_EP1_IN Vector16C
#define vhalIrq_OTG_HS_WKUP Vector170
#define vhalIrq_OTG_HS Vector174
#define vhalIrq_DCMI Vector178
#define vhalIrq_CRYP Vector17C
#define vhalIrq_HASH_RNG Vector180
#define vhalIrq_FPU Vector184
#define vhalIrq_UART7 Vector188
#define vhalIrq_UART8 Vector18C
#define vhalIrq_SPI4 Vector190
#define vhalIrq_SPI5 Vector194
#define vhalIrq_SPI6 Vector198
#define vhalIrq_SAI1 Vector19C
#define vhalIrq_LCD_TFT Vector1A0
#define vhalIrq_LCD_TFT_ER Vector1A4
#define vhalIrq_DMA2D Vector1A8

#endif


#endif
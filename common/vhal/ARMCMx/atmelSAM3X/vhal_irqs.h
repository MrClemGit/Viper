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

#define vhalIrqEnable(irqn) NVIC_EnableIRQ(irqn)
#define vhalIrqDisable(irqn) NVIC_DisableIRQ(irqn)
#define vhalIrqSetPriority(iqrn,prio) NVIC_SetPriority(irqn,prio)
#define vhalIrqClearPending(IRQn) NVIC->ICPR[((uint32_t)(IRQn) >> 5)] = (1 << ((uint32_t)(IRQn) & 0x1F))
#define vhalIrqInit(InitStruct) NVIC_Init(InitStruct)



#define vhalIrq_SUPC         Vector40
#define vhalIrq_RSTC    Vector44
#define vhalIrq_RTC   Vector48
#define vhalIrq_RTT     Vector4C
#define vhalIrq_WDT Vector50
#define vhalIrq_PMC Vector54
#define vhalIrq_EFC0 Vector58
#define vhalIrq_EFC1 Vector5C
#define vhalIrq_UART Vector60
#define vhalIrq_SMC Vector64
#define vhalIrq_SDRAMC Vector68
#define vhalIrq_PIOA Vector6C
#define vhalIrq_PIOB Vector70
#define vhalIrq_PIOC Vector74
#define vhalIrq_PIOD Vector78
#define vhalIrq_PIOE Vector7C
#define vhalIrq_PIOF Vector80
#define vhalIrq_USART0 Vector84
#define vhalIrq_USART1 Vector88
#define vhalIrq_USART2 Vector8C
#define vhalIrq_USART3 Vector90
#define vhalIrq_HSMCI Vector94
#define vhalIrq_I2C0 Vector98
#define vhalIrq_I2C1 Vector9C
#define vhalIrq_SPI0 VectorA0
#define vhalIrq_SPI1 VectorA4
#define vhalIrq_SSC VectorA8
#define vhalIrq_TC0 VectorAC
#define vhalIrq_TC1 VectorB0
#define vhalIrq_TC2 VectorB4
#define vhalIrq_TC3 VectorB8
#define vhalIrq_TC4 VectorBC
#define vhalIrq_TC5 VectorC0
#define vhalIrq_TC6 VectorC4
#define vhalIrq_TC7 VectorC8
#define vhalIrq_TC8 VectorCC
#define vhalIrq_PWM VectorD0
#define vhalIrq_ADC VectorD4
#define vhalIrq_DAC VectorD8
#define vhalIrq_DMAC VectorDC
#define vhalIrq_UOTGHS VectorE0
#define vhalIrq_TRNG VectorE4
#define vhalIrq_EMAC VectorE8
#define vhalIrq_CAN0 VectorEC
#define vhalIrq_CAN1 VectorF0
#endif


#endif
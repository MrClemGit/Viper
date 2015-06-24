#include "vhal.h"



#if BOARD_HAS_RNG==1
#ifdef STM32F2XX
#include "stm32f2xx_rcc.h"
#include "stm32f2xx_rng.h"
#else
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_rng.h"
#endif

uint32_t vhalRngGenerate(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
    RNG->CR |= RNG_CR_RNGEN;
    while (!(RNG->SR & (RNG_SR_DRDY)));
    uint32_t res = RNG->DR;
    RCC->AHB2ENR &= ~RCC_AHB2ENR_RNGEN;    
    RNG->CR &= ~RNG_CR_RNGEN;
    return res;
}


int vhalRngSeed(uint32_t seed) {
    return 0;
}

#else


int vhalInitRNG(void*data){
	//seed by uid
	(void)data;
    vhalRngSeed(*((uint32_t *)0x1FFF7A10));
    return 0;
}

#endif


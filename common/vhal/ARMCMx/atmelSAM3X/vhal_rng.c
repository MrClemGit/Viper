#include "vhal.h"
//#include "inc/sam3x8e.h"
#include "hal.h" //<--- just for pmc_clk



#if BOARD_HAS_RNG==1



#define TRNG_CR_KEY_PASSWD    TRNG_CR_KEY(0x524E47)

uint32_t vhalRngGenerate(void) {
    TRNG->TRNG_CR = TRNG_CR_ENABLE | TRNG_CR_KEY_PASSWD;
    while (!(TRNG->TRNG_ISR & TRNG_ISR_DATRDY));
    return TRNG->TRNG_ODATA;
}


int vhalRngSeed(uint32_t seed) {
    (void)seed;
    return 0;
}


int vhalInitRNG(void*data){
    //seed by uid
    (void)data;
    pmc_enable_periph_clk(ID_TRNG);
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


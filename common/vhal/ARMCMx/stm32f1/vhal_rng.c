#include "vhal.h"
#include "vhal_nfo.h"



#if BOARD_HAS_RNG==1


uint32_t vhalRngGenerate(void) {
    return 0;
}


int vhalRngSeed(uint32_t seed) {
    return 0;
}

#else


int vhalInitRNG(void*data){
	//seed by uid
	(void)data;
    vhalRngSeed((uint32_t)(*(UID_ADDR)));
    return 0;
}

#endif


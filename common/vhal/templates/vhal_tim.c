#include "vhal.h"
#include "vhal_gpio.h"
#include "vhal_tim.h"
#include "vhal_irqs.h"


int vhalInitTIM(void *data) {
    (void)data;
    return 0;
} 



int vhalPwmStart(int vpin, uint32_t period, uint32_t pulse) {
	(void)vpin;
	(void)period;
	(void)pulse;

	return 0;
}


int vhalIcuStart(int vpin, vhalIcuConf *conf) {
	(void)vpin;
	(void)conf;
	return 0;
}

int vhalHtmGetFreeTimer(){
	return 0;
}

int vhalHtmOneShot(uint32_t tm, uint32_t delay, htmFn fn, void *args){
	return 0;
}


int vhalHtmRecurrent(uint32_t tm, uint32_t delay, htmFn fn, void *args){
	return 0;
}





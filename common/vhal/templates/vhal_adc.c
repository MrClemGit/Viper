#include "vhal.h"
#include "vhal_gpio.h"


int vhalInitADC(void *data) {
	(void)data;

	return 0;
}



int vhalAdcGetPeripheralForPin(int vpin) {
	if (PIN_CLASS(vpin) != PINCLASS_ANALOG)
		return -1;
	return PIN_CLASS_DATA1(vpin);
}

int vhalAdcInit(uint32_t adc, vhalAdcConf *conf) {
	return 0;
}

int vhalAdcPrepareCapture(uint32_t adc, vhalAdcCaptureInfo *info) {
	return 0;
}


/*TODO: add support for more than 1 adc */
int vhalAdcRead(uint32_t adc, vhalAdcCaptureInfo *info) {
	return 0;
}

int vhalAdcDone(uint32_t adc) {
	return 0;
}


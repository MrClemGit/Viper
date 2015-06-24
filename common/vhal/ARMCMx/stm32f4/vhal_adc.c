#include "vhal.h"
#include "vhal_gpio.h"
#include "hal.h"


ADCDriver *adcdrv[3];
vhalAdcCaptureInfo *adccapt[3];
uint16_t adcstatus[3];
VThread *adcthread[3];
ADCConversionGroup adccg[3];

int vhalInitADC(void *data) {
	(void)data;

#if STM32_ADC_USE_ADC1
	adcdrv[PERIPHERAL_ID(1)] = &ADCD1;
#endif
#if STM32_ADC_USE_ADC2
	adcdrv[PERIPHERAL_ID(2)] = &ADCD2;
#endif
#if STM32_ADC_USE_ADC3
	adcdrv[PERIPHERAL_ID(3)] = &ADCD3;
#endif
	return 0;
}


const uint16_t _adc_smp[] STORED = {15, 27, 40, 68, 96, 124, 156, 492};


int vhalAdcGetPeripheralForPin(int vpin) {
	if (PIN_CLASS(vpin) != PINCLASS_ANALOG)
		return -1;
	return PIN_CLASS_DATA1(vpin);
}

int vhalAdcInit(uint32_t adc, vhalAdcConf *conf) {
	if (adc >= PERIPHERAL_NUM(adc))
		return -1;
	int adcid = GET_PERIPHERAL_ID(adc, adc);
	if (!adcstatus[adcid]) {
		int cycles, ok_cycles = -1;
		if (conf != NULL) {
			for (cycles = 0; cycles < 8; cycles++) {//cycles per sample
				uint32_t cur_freq = _system_frequency / VHAL_ADC_PRESCALER / _adc_smp[cycles];
				if (conf->samples_per_second <= cur_freq) {
					ok_cycles = cycles;
				} else {
					if (ok_cycles < 0) {
						//unsupported sample frequency
						return -1;
					} else {
						break;
					}
				}
			}
			conf->samples_per_second = _system_frequency / VHAL_ADC_PRESCALER / _adc_smp[ok_cycles];
		} else ok_cycles = 0;
		/* activates ADC @ 21 MHz (APB2/DIV4) */
		/* TODO: add trigger configuration */
		/* TODO: add resolution config */
		adcStart(adcdrv[adcid], NULL);
		adcstatus[adcid] = ((ok_cycles) << 8) | 1;
	}
	return 0;
}

int vhalAdcPrepareCapture(uint32_t adc, vhalAdcCaptureInfo *info) {
	int i;
	ADCConversionGroup *cg = &adccg[GET_PERIPHERAL_ID(adc, adc)];
	if (info->npins > 16 || info->npins <= 0)
		return -1;
	cg->smpr1 = 0;
	cg->smpr2 = 0;
	cg->num_channels = info->npins;
	cg->end_cb = NULL;
	cg->error_cb = NULL;
	cg->cr1 = 0;
	cg->sqr3 = 0;
	cg->sqr2 = 0;
	cg->sqr1 = (info->npins-1) << 20;
	int cycles = adcstatus[GET_PERIPHERAL_ID(adc, adc)] >> 8;
	for (i = 0; i < info->npins; i++) {
		uint16_t vpin = info->pins[i];
		if (PIN_CLASS(vpin) != PINCLASS_ANALOG)
			return -2;
		int adc_ch = PIN_CLASS_DATA0(vpin);		
		palSetPadMode(CPIN_PORT(vpin), PIN_PAD(vpin), PAL_MODE_INPUT_ANALOG);

		//group capture
		if (i <= 5) {
			cg->sqr3 |= (adc_ch) << (5 * i);
		} else if (i <= 11) {
			cg->sqr2 |= (adc_ch) << (5 * (i - 6));
		} else {
			cg->sqr1 |= (adc_ch) << (5 * (i - 12));
		}
		//set sample rate for each pin
		if (adc_ch <= 9) {
			cg->smpr1 |= (cycles) << (3 * adc_ch);
		} else {
			cg->smpr2 |= (cycles) << (3 * (adc_ch - 10));
		}
	}
	cg->cr2 = ADC_CR2_SWSTART;
	if (info->samples > 1 && info->samples % 2 != 0)
		info->samples++;
	info->sample_size = 2;
	return info->sample_size * info->samples * info->npins;
}


typedef void (*adccallback_t)(ADCDriver *adcp, adcsample_t *buffer, size_t n);

void adccbk1(ADCDriver *adcp, adcsample_t *buffer, uint32_t n) {
	(void)adcp;
	(void)n;
	vhalAdcCaptureInfo *info = adccapt[GET_PERIPHERAL_ID(adc, 0)];
	info->half_buffer = buffer;
	if (info->callback) {
		if (info->callback(0, info)) {
			/*TODO: stop the capture!*/
		}
	}
}

/*TODO: add support for more than 1 adc */
int vhalAdcRead(uint32_t adc, vhalAdcCaptureInfo *info) {
	int adcid = GET_PERIPHERAL_ID(adc, adc);
	ADCConversionGroup *cg = &adccg[adcid];
	switch (info->capture_mode) {
		case ADC_CAPTURE_SINGLE:
			cg->circular = 0;
			adcConvert(adcdrv[adcid], cg, info->buffer, info->samples);
			break;
		case ADC_CAPTURE_CONTINUOUS:
			cg->circular = 1;
			cg->end_cb = adccbk1;//<< this index should be dependent on uint32_t adc!!
			adccapt[adcid] = info;
			adcConvert(adcdrv[adcid], cg, info->buffer, info->samples);
			/*TODO call adcStartConversion and store suspend the current thread */
			break;
	}
	return 0;
}

int vhalAdcDone(uint32_t adc) {
	int adcid = GET_PERIPHERAL_ID(adc, adc);
	if (adcstatus[adcid]) {
		adcStop(adcdrv[GET_PERIPHERAL_ID(adc, adc)]);
		adcstatus[adcid] = 0;
	}
	return 0;
}


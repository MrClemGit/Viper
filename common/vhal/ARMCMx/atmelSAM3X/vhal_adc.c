#include "vhal.h"
#include "hal.h"
#include "vhal_gpio.h"


ADCDriver *adcdrv;
ADCConfig adccfg;
vhalAdcCaptureInfo *adccapt;
uint8_t adcstatus;
VThread *adcthread;
ADCConversionGroup adccg;


int vhalInitADC(void *data) {
	(void)data;
	adcdrv = &ADCD1;
	adcstatus = 0;
	return 0;
}



int vhalAdcGetPeripheralForPin(int vpin) {
	if (PIN_CLASS(vpin) != PINCLASS_ANALOG)
		return -1;
	return PIN_CLASS_DATA1(vpin);
}

int vhalAdcInit(uint32_t adc, vhalAdcConf *conf) {
	if (adc >= PERIPHERAL_NUM(adc))
		return -1;
	if (!adcstatus) {
		//TODO: config adc prescaler based on conf. At the moment always return 1000000
		if (conf != NULL) {
			if (conf->samples_per_second > 1000000)
				return -1;
			conf->samples_per_second = 1000000;
		}
		adccfg.samplerate = 0; //TODO: adc_lld.c ignores it atm
		adccfg.resolution = ADC_HI_RES;
		adccfg.startup = ADC_STARTUP_TIME_0;
		adccfg.trigger = 0;
		adcStart(adcdrv, &adccfg);
		adcstatus =  1;
	}
	return 0;
}

int vhalAdcPrepareCapture(uint32_t adc, vhalAdcCaptureInfo *info) {
	int i;
	ADCConversionGroup *cg = &adccg;
	if (info->npins > 16 || info->npins <= 0)
		return -1;

	cg->num_channels = info->npins;
	cg->end_cb = NULL;
	cg->error_cb = NULL;

	cg->channels = 0;
	cg->seq1 = cg->seq2 = 0;
	for (i = 0; i < info->npins; i++) {
		uint16_t vpin = info->pins[i];
		if (PIN_CLASS(vpin) != PINCLASS_ANALOG)
			return -2;
		vhalPinSetMode(vpin, PINMODE_INPUT_ANALOG);
		int adc_ch = PIN_CLASS_DATA0(vpin);
		cg->channels |= (1 << adc_ch); //set channel on in channel mask
		if (i < 8) {
			cg->seq1 |= (adc_ch) << (4 * i);
		} else {
			cg->seq2 |= (adc_ch) << (4 * (i - 8));
		}
	}
	info->sample_size = 2;
	if (info->samples > 1 && info->samples % 2 != 0)
		info->samples++;
	return info->sample_size * info->samples * info->npins;
}


/*TODO: add support for more than 1 adc */
int vhalAdcRead(uint32_t adc, vhalAdcCaptureInfo *info) {
	ADCConversionGroup *cg = &adccg;
	switch (info->capture_mode) {
		case ADC_CAPTURE_SINGLE:
			cg->circular = 0;
			if (cg->num_channels == 1)
				adcConvert(adcdrv, cg, info->buffer, info->samples);
			else {
				//sequence capture must be simulated, sam3 adc has too many constraints on channels [RM pag 1325>>]
				//yeah, ugly :(
				int i, j, ch;
				uint16_t *buf = (uint16_t*)info->buffer;
				for (j = 0; j < info->samples; j++) {
					for (i = 0; i < info->npins; i++) {
						if (i < 8) {
							ch = (cg->seq1 >> (4 * i)) & 0xf;
						} else {
							ch = (cg->seq1 >> (4 * (i - 8))) & 0xf;
						}
						cg->channels = (1 << ch);
						adcConvert(adcdrv, cg, buf++, 1);
					}
				}
			}
			break;
		case ADC_CAPTURE_CONTINUOUS:
			cg->circular = 1;
			//TODO: implement  callback mechanism for continuous mode
			cg->end_cb = NULL;//adccbk1;//<< this index should be dependent on uint32_t adc!!
			adccapt = info;
			adcConvert(adcdrv, cg, info->buffer, info->samples);
			/*TODO call adcStartConversion and store suspend the current thread */
			break;
	}
	return 0;
}

int vhalAdcDone(uint32_t adc) {
	if (adcstatus) {
		adcStop(adcdrv);
		adcstatus = 0;
	}
	return 0;
}


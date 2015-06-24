#include "vhal.h"
#include "hal.h"
#include "vhal_gpio.h"



//TODO: dinamically set channels by EXT_INTERRUPTS_NEEDED
EXTConfig _extcfg = {
	{
		EXT_CHANNEL_CFG(0, 0, 0, NULL),
		EXT_CHANNEL_CFG(0, 0, 0, NULL),
		EXT_CHANNEL_CFG(0, 0, 0, NULL),
		EXT_CHANNEL_CFG(0, 0, 0, NULL),
		EXT_CHANNEL_CFG(0, 0, 0, NULL),
		EXT_CHANNEL_CFG(0, 0, 0, NULL),
		EXT_CHANNEL_CFG(0, 0, 0, NULL),
		EXT_CHANNEL_CFG(0, 0, 0, NULL),
		EXT_CHANNEL_CFG(0, 0, 0, NULL),
		EXT_CHANNEL_CFG(0, 0, 0, NULL),
		EXT_CHANNEL_CFG(0, 0, 0, NULL),
		EXT_CHANNEL_CFG(0, 0, 0, NULL),
		EXT_CHANNEL_CFG(0, 0, 0, NULL),
		EXT_CHANNEL_CFG(0, 0, 0, NULL),
		EXT_CHANNEL_CFG(0, 0, 0, NULL),
		EXT_CHANNEL_CFG(0, 0, 0, NULL)

	}
};


typedef struct _vhalext {
	extCbkFn fns[EXT_INTERRUPTS_NEEDED];
	uint16_t pins[EXT_INTERRUPTS_NEEDED];
	uint8_t mode[EXT_INTERRUPTS_NEEDED];
} EXTInfo;


const uint8_t _extmodes[] STORED = {EXT_CH_MODE_FALLING_EDGE, EXT_CH_MODE_RISING_EDGE, EXT_CH_MODE_BOTH_EDGES};
const uint8_t _extnummodes = sizeof(_extmodes) / sizeof(_extmodes[0]);


EXTInfo _ext;


void vhal_ext_callback(EXTDriver *extp, expchannel_t channel, int dir) {
	(void)extp;
	if (_ext.fns[channel]) {
		_ext.fns[channel](channel, dir ? 1 : 0);
	}
}

int vhalInitEXT(void *data) {
	(void)data;
	memset(&_ext, 0, sizeof(EXTInfo));
	extStart(&EXTD1, &_extcfg);
	return 0;
}



int vhalPinAttachInterrupt(int vpin, int mode, extCbkFn fn) {
	int slot = -1;
	int i;
	int pad = PIN_PAD(vpin);
	int port = PIN_PORT_NUMBER(vpin);

	if (!PIN_HAS_PRPH(vpin, PRPH_EXT)) {
		goto ext_ret;
	}

	//search slot
	for (i = 0; i < EXT_INTERRUPTS_NEEDED; i++) {
		if (!_extcfg.channels[i].enabled) {
			slot = i;
		} else {
			if (_extcfg.channels[i].ioport == port && _extcfg.channels[i].pad == pad) {
				slot = i;
				break;
			}
		}
	}

	if (slot < 0)
		goto ext_ret;

	if (fn) {
		int pmode = mode & 0xf;
		_ext.fns[slot] = fn;
		_ext.pins[slot] = vpin;
		_ext.mode[slot] |= mode & (0xf0);
		mode = mode & 0xf0;
		mode = (_ext.mode[slot] == PINMODE_EXT_FALLING) ? 0 : ((_ext.mode[slot] == PINMODE_EXT_RISING) ? 1 : 2);

		_extcfg.channels[slot].mode = _extmodes[mode];
		_extcfg.channels[slot].cb = vhal_ext_callback;
		_extcfg.channels[slot].ioport = port;
		_extcfg.channels[slot].pad = pad;

		vosSysLock();
		vhalPinSetMode(vpin, pmode);
		extSetChannelModeI(&EXTD1, slot, &_extcfg.channels[slot]);
		extChannelEnableI(&EXTD1, slot);
		vosSysUnlock();
	} else {
		vosSysLock();
		extChannelDisableI(&EXTD1, slot);
		vosSysUnlock();
		_extcfg.channels[slot].mode = EXT_CH_MODE_DISABLED;
		_ext.fns[slot] = NULL;
		_ext.mode[slot] = 0;
	}

ext_ret:
	return slot;
}
#include "vhal.h"
#include "hal.h"
#include "vhal_gpio.h"



EXTConfig _extcfg = {
    {
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
        {EXT_CH_MODE_DISABLED, NULL},
    }
};

typedef struct _vhalext {
    extCbkFn fns[16];
    uint16_t pins[16];
    uint8_t mode[16];
} EXTInfo;


const uint32_t _extport[] STORED = {EXT_MODE_GPIOA, EXT_MODE_GPIOB, EXT_MODE_GPIOC, EXT_MODE_GPIOD};
const uint8_t _extmodes[] STORED = {EXT_CH_MODE_FALLING_EDGE, EXT_CH_MODE_RISING_EDGE, EXT_CH_MODE_BOTH_EDGES};
const uint8_t _extnummodes = sizeof(_extmodes) / sizeof(_extmodes[0]);

EXTInfo _ext;

void vhal_ext_callback(EXTDriver *extp, expchannel_t channel) {
    (void)extp;
    vosSysLockIsr();
    int dir = 0;
    if (_ext.mode[channel] == PINMODE_EXT_BOTH) {
        if (vPinRead(CPIN_PORT(_ext.pins[channel]), PIN_PAD(_ext.pins[channel]))) {
            dir = 1;
        }
    } else if (_ext.mode[channel] == PINMODE_EXT_RISING) {
        dir = 1;
    }
    vosSysUnlockIsr();
    if (_ext.fns[channel])
        _ext.fns[channel](channel, dir);
}

int vhalInitEXT(void *data) {
    (void)data;
    memset(&_ext, 0, sizeof(EXTInfo));
    extStart(&EXTD1, &_extcfg);
    return 0;
}

int vhalPinAttachInterrupt(int vpin, int mode, extCbkFn fn) {
    int prt = _extport[PIN_PORT_NUMBER(vpin)];
    int pad = PIN_PAD(vpin);

    if (!PIN_HAS_PRPH(vpin, PRPH_EXT)) {
        pad = -1;
        goto ext_ret;
    }


    if (fn) {
        int pmode = mode & 0xf;
        _ext.fns[pad] = fn;
        _ext.pins[pad] = vpin;
        _ext.mode[pad] |= mode & (0xf0);
        mode = mode & 0xf0;
        mode = (_ext.mode[pad] == PINMODE_EXT_FALLING) ? 0 : ((_ext.mode[pad] == PINMODE_EXT_RISING) ? 1 : 2);
        _extcfg.channels[pad].mode = _extmodes[mode] | prt | EXT_CH_MODE_AUTOSTART;
        _extcfg.channels[pad].cb = vhal_ext_callback;
        vosSysLock();
        //TODO: get input mode from pmode
        //vhalPinSetMode(vpin,PINMODE_INPUT_PULLUP);
        extSetChannelModeI(&EXTD1, pad, &_extcfg.channels[pad]);
        extChannelEnableI(&EXTD1, pad);
        vosSysUnlock();
    } else {
        vosSysLock();
        extChannelDisableI(&EXTD1, pad);
        vosSysUnlock();
        _extcfg.channels[pad].mode = EXT_CH_MODE_DISABLED;
        _ext.fns[pad] = NULL;
        _ext.mode[pad] = 0;
    }

ext_ret:
    return pad;
}
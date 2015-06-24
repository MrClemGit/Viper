/*
{{ project }}
Copyright (C) {{ year }}  {{ organization }}

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "vhal.h"


int vhalGetPeripheralForPin(int vpin, int pinclass) {
    int cls = PIN_CLASS(vpin);
    int pad = PIN_CLASS_PAD(vpin);
    if (pad < PIN_CLASS_NUM(cls) && pinclass == cls) {
        return PIN_CLASS_DATA1(vpin);
    }
    return -1;
}

int vhalUnsupportedPRPH(void *data) {
    (void)data;
    return 1;
}

extern int vhalInitGPIO(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitEXT(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitADC(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitSPI(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitI2C(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitI2S(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitSER(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitCDC(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitPWM(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitICU(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitCAN(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitDAC(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitMAC(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitSDC(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitUSB(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitHTM(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitTIM(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitRTC(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitRNG(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitCRYP(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitFLASH(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitNFO(void *data) ALIASED(vhalUnsupportedPRPH);
extern int vhalInitDMA(void *data) ALIASED(vhalUnsupportedPRPH);



void blink_off(int vpin) {
    //vhalPinSetMode(0, 0);
    //vhalPinWrite(0, 1);

    vhalPinSetMode(vpin, PINMODE_OUTPUT_PUSHPULL);
    vhalPinWrite(vpin, 1);
}

void blink(int vpin, int n) {

    vhalPinSetMode(vpin, PINMODE_OUTPUT_PUSHPULL);
    vhalPinWrite(vpin, 1);
    //vosThSleep(TIME_U(1, SECONDS));
    while (n) {
        vhalPinWrite(vpin, 0);
        //vosThSleep(TIME_U(500, MILLIS));
        vhalPinWrite(vpin, 1);
        //vosThSleep(TIME_U(500, MILLIS));
        n--;
    }
    //vosThSleep(TIME_U(1, SECONDS));

}

int vhalInit(void *data) {
    (void)data;


#if VHAL_DMA
    vhalInitDMA(data);
#endif


#if VHAL_GPIO
    vhalInitGPIO(data);
#endif

#if VHAL_CDC
    vhalInitCDC(data);
#endif


#if VHAL_SER
    vhalInitSER(data);
#endif


#if VHAL_ADC
    vhalInitADC(data);
#endif

#if VHAL_HTM
    vhalInitHTM(data);
#endif


//TODO: change flags for stm32
#if VHAL_TIM
vhalInitTIM(data);
#endif


#if VHAL_PWM
    vhalInitPWM(data);
#endif

#if VHAL_ICU
    vhalInitICU(data);
#endif

#if VHAL_EXT
    vhalInitEXT(data);
#endif

#if VHAL_SPI
    vhalInitSPI(data);
#endif

#if VHAL_I2C
    vhalInitI2C(data);
#endif

#if VHAL_CAN
    vhalInitCAN(data);
#endif

#if VHAL_DAC
    vhalInitDAC(data);
#endif

#if VHAL_MAC
    vhalInitMAC(data);
#endif

#if VHAL_SDC
    vhalInitSDC(data);
#endif

#if VHAL_USB
    vhalInitUSB(data);
#endif

#if VHAL_RTC
    vhalInitRTC(data);
#endif

#if VHAL_RNG
    vhalInitRNG(data);
#endif

#if VHAL_CRYP
    vhalInitCRYP(data);
#endif

#if VHAL_FLASH
    vhalInitFLASH(data);
#endif


#if VHAL_NFO
    vhalInitNFO(data);
#endif

    return 0;
}



/* ========================================================================
    RNG - Software
   ======================================================================== */


#if BOARD_HAS_RNG==0
//software RNG
/* adapted from: http://burtleburtle.net/bob/rand/smallprng.html */


typedef uint32_t  u4;
typedef struct ranctx {
    u4 a;
    u4 b;
    u4 c;
    u4 d;
} ranctx;

#define rot(x,k) (((x)<<(k))|((x)>>(32-(k))))

static ranctx randstr;

uint32_t vhalRngGenerate() {

    ranctx *x = &randstr;
    vosSysLock();
    u4 e = x->a - rot(x->b, 27);
    x->a = x->b ^ rot(x->c, 17);
    x->b = x->c + x->d;
    x->c = x->d + e;
    x->d = e + x->a;
    vosSysUnlock();
    return x->d;
}

int vhalRngSeed(uint32_t seed) {
    u4 i;
    ranctx *x = &randstr;
    x->a = 0xf1ea5eed, x->b = x->c = x->d = seed;
    for (i = 0; i < 20; ++i) {
        (void)vhalRngGenerate();
    }
    return seed;
}


#endif



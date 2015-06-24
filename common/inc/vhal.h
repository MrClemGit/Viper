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


#ifndef __VHAL__
#define __VHAL__


#include "vosal.h"
#include "board_mcu.h"


int vhalInit(void *data);


/* ========================================================================
    PIN MAPPING
   ======================================================================== */

#include "vhal_pins.h"

#define PERIPHERAL_ID(n) ((uint8_t)((n)-1))
#define GET_PERIPHERAL_ID(name,vprph) _vhal_ ## name ## _map[vprph]
#define PERIPHERAL_NUM(name) (_vhal_ ## name ## _num)
#define BEGIN_PERIPHERAL_MAP(name) const uint8_t _vhal_ ## name ## _map[] STORED = {
#define END_PERIPHERAL_MAP(name) }; \
  const uint8_t _vhal_ ## name ## _num = sizeof(_vhal_ ## name ## _map)
#define DECLARE_PERIPHERAL_MAP(name) extern const uint8_t _vhal_ ## name ## _map[]; \
  extern const uint8_t _vhal_ ## name ## _num


int vhalGetPeripheralForPin(int vpin, int pinclass);

/* ========================================================================
    GPIO
   ======================================================================== */

#define PINMODE_NONE              0

#define PINMODE_INPUT_PULLNONE    1
#define PINMODE_INPUT_PULLUP      2
#define PINMODE_INPUT_PULLDOWN    3

#define PINMODE_OUTPUT_PUSHPULL   4
#define PINMODE_OUTPUT_OPENDRAIN  5
#define PINMODE_OUTPUT_HIGHDRIVE  6

#define PINMODE_INPUT_ANALOG      7
#define PINMODE_GPIO_MODES        7

#define PINMODE_ALTERNATE_FUNCTION 15

#define PINMODE_EXT_FALLING     16
#define PINMODE_EXT_RISING      32
#define PINMODE_EXT_BOTH        (PINMODE_EXT_RISING|PINMODE_EXT_FALLING)



int vhalPinSetMode(int vpin, int mode);
int vhalPinRead(int vpin);
int vhalPinWrite(int vpin, int value);
int vhalPinToggle(int vpin);
void vhalPinFastSet(void *port, int pad);
void vhalPinFastClear(void *port, int pad);
int vhalPinFastRead(void *port, int pad);
void *vhalPinGetPort(int vpin);
int vhalPinGetPad(int vpin);
int vhalPinSetToPeripheral(int vpin, int prph, uint32_t prms);

/* ========================================================================
    EXT
   ======================================================================== */

typedef void (*extCbkFn)(int slot, int dir);

int vhalPinAttachInterrupt(int vpin, int mode, extCbkFn fn);

/* ========================================================================
    ADC
   ======================================================================== */



typedef struct _vhal_adc_capture {
  uint32_t samples;
  uint16_t *pins;
  uint8_t npins;
  uint8_t sample_size;
  uint8_t capture_mode;
  uint8_t trigger_mode;
  uint16_t trigger_vpin;
  void *buffer;
  void *half_buffer;
  int (*callback)(uint32_t, struct _vhal_adc_capture*);
} vhalAdcCaptureInfo;
typedef int (*adcCbkFn)(uint32_t adc, vhalAdcCaptureInfo *nfo);



typedef struct _vhal_adc_conf {
  uint32_t samples_per_second;
  uint32_t resolution;
} vhalAdcConf;


#define ADC_CAPTURE_SINGLE 0
#define ADC_CAPTURE_CONTINUOUS 1

DECLARE_PERIPHERAL_MAP(adc);

int vhalAdcInit(uint32_t adc, vhalAdcConf *conf);
int vhalAdcGetPeripheralForPin(int vpin);
int vhalAdcPrepareCapture(uint32_t adc, vhalAdcCaptureInfo *info);
int vhalAdcRead(uint32_t adc, vhalAdcCaptureInfo *info);
int vhalAdcDone(uint32_t adc);


/* ========================================================================
    PWM
   ======================================================================== */

DECLARE_PERIPHERAL_MAP(pwm);
int vhalPwmStart(int vpin, uint32_t period, uint32_t pulse);


/* ========================================================================
    ICU
   ======================================================================== */

#define ICU_TRIGGER_LOW 0
#define ICU_TRIGGER_HIGH 1

#define ICU_MODE_PULSE  0
#define ICU_MODE_PERIOD 1
#define ICU_MODE_BOTH   2

#define ICU_INPUT_PULLUP 0
#define ICU_INPUT_PULLDOWN 1

#define ICU_CFG(mode, trigger, filter, input) (((filter)<<8)| ((input)<<3)|((mode)<<1)|(trigger))
#define ICU_CFG_GET_MODE(cfg) (((cfg)>>1)&0x3)
#define ICU_CFG_GET_TRIGGER(cfg) ((cfg)&1)
#define ICU_CFG_GET_INPUT(cfg) (((cfg)>>3)&1)
#define ICU_CFG_GET_FILTER(cfg) ((cfg)>>8)


#define ICU_CALLBACK_RESET_BUFFER  1
#define ICU_CALLBACK_OK 0

DECLARE_PERIPHERAL_MAP(icu);

typedef int (*icuCbkFn)(int dir, uint32_t time, int pos);
typedef void (*icuCbkEnd)(void *conf);

typedef struct _vhal_icu_conf {
  uint32_t time_window;
  uint16_t cfg;
  uint16_t bufsize;
  uint32_t *buffer;
  icuCbkFn callback;
  icuCbkEnd endcbk;
  void *args;
} vhalIcuConf;

int vhalIcuStart(int vpin, vhalIcuConf *conf);


/* ========================================================================
    HTM
   ======================================================================== */

DECLARE_PERIPHERAL_MAP(htm);

typedef void (*htmFn)(uint32_t tm, void *args);

int vhalHtmGetFreeTimer(void);
int vhalHtmOneShot(uint32_t tm, uint32_t delay, htmFn fn, void *args);
int vhalHtmRecurrent(uint32_t tm, uint32_t period, htmFn fn, void *args);
int vhalInitTIM(void *data);


/* ========================================================================
    SER
   ======================================================================== */


#define SERIAL_PARITY_NONE 0
#define SERIAL_PARITY_EVEN 1
#define SERIAL_PARITY_ODD  2

#define SERIAL_STOP_ONE 0
#define SERIAL_STOP_ONEHALF 1
#define SERIAL_STOP_TWO  2

#define SERIAL_BITS_8 0
#define SERIAL_BITS_7 1


DECLARE_PERIPHERAL_MAP(serial);

int vhalSerialInit(uint32_t ser, uint32_t baud, uint32_t parity, uint32_t stop, uint32_t bits, uint32_t rxpin,uint32_t txpin);
int vhalSerialRead(uint32_t ser, uint8_t *buf, uint32_t len);
int vhalSerialWrite(uint32_t ser, uint8_t *buf, uint32_t len);
int vhalSerialAvailable(uint32_t ser);
int vhalSerialDone(uint32_t ser);


/* ========================================================================
    SPI
   ======================================================================== */

typedef struct _vhal_spi_conf {
  uint32_t clock;
  uint16_t miso;
  uint16_t mosi;
  uint16_t sclk;
  uint16_t nss;
  uint8_t mode;
  uint8_t bits;
  uint8_t master;
  uint8_t unused;
} vhalSpiConf;

DECLARE_PERIPHERAL_MAP(spi);

#define SPI_MODE_LOW_FIRST 0
#define SPI_MODE_LOW_SECOND 1
#define SPI_MODE_HIGH_FIRST 2
#define SPI_MODE_HIGH_SECOND 3
#define SPI_BITS_8 0
#define SPI_BITS_16 1
#define SPI_BITS_32 2

int vhalSpiInit(uint32_t spi, vhalSpiConf *conf);
int vhalSpiLock(uint32_t spi);
int vhalSpiUnlock(uint32_t spi);
int vhalSpiSelect(uint32_t spi);
int vhalSpiUnselect(uint32_t spi);
int vhalSpiExchange(uint32_t spi, void *tosend, void *toread, uint32_t blocks);
int vhalSpiDone(uint32_t spi);


/* ========================================================================
    NFO
   ======================================================================== */

int vhalNfoGetUIDLen(void);
int vhalNfoGetUID(uint8_t *buf);
uint8_t *vhalNfoGetUIDStr(void);


/* ========================================================================
    FLASH
   ======================================================================== */

int vhalFlashGetSector(void *addr);
int vhalFlashErase(void *addr, uint32_t size);
int vhalFlashWrite(void *addr, uint8_t *data, uint32_t len);
void *vhalFlashAlignToSector(void *addr);


/* ========================================================================
    RNG
   ======================================================================== */

uint32_t vhalRngGenerate(void);
int vhalRngSeed(uint32_t seed);


/* ========================================================================
    ERRORS
   ======================================================================== */

#include "ptypes.h"

#define VHAL_OK                     ERR_OK
#define VHAL_GENERIC_ERROR         -ERR_PERIPHERAL_ERROR_EXC
#define VHAL_INVALID_PIN           -ERR_PERIPHERAL_INVALID_PIN_EXC
#define VHAL_HARDWARE_STATUS_ERROR -ERR_PERIPHERAL_INVALID_HARDWARE_STATUS_EXC


#endif
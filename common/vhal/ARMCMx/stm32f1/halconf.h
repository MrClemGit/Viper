/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/


#ifndef _HALCONF_H_
#define _HALCONF_H_

#include "board_mcu.h"
#include "mcuconf.h"

#define HAL_USE_TM                  FALSE
#define HAL_USE_MMC_SPI             FALSE
#define MMC_NICE_WAITING            TRUE



#if VHAL_GPIO
#define HAL_USE_PAL                 TRUE
#else
#define HAL_USE_PAL                 FALSE
#endif


#if VHAL_ADC
#define HAL_USE_ADC                 TRUE
#define ADC_USE_MUTUAL_EXCLUSION    FALSE
#define ADC_USE_WAIT                TRUE
#else
#define HAL_USE_ADC                 FALSE
#endif


#if VHAL_CAN
#define HAL_USE_CAN                 TRUE
#define CAN_USE_SLEEP_MODE          TRUE
#else
#define HAL_USE_CAN                 FALSE
#endif


#if VHAL_EXT
#define HAL_USE_EXT                 TRUE
#else
#define HAL_USE_EXT                 FALSE
#endif


#if VHAL_HTM
#define HAL_USE_GPT                 FALSE
#else
#define HAL_USE_GPT                 FALSE
#endif


#if VHAL_I2C
#define HAL_USE_I2C                 TRUE
#define I2C_USE_MUTUAL_EXCLUSION    TRUE
#else
#define HAL_USE_I2C                 FALSE
#endif


#if VHAL_ICU
#define HAL_USE_ICU                 TRUE
#else
#define HAL_USE_ICU                 FALSE
#endif



#if VHAL_MAC
#define HAL_USE_MAC                 TRUE
#define MAC_USE_ZERO_COPY           FALSE
#define MAC_USE_EVENTS              TRUE
#else
#define HAL_USE_MAC                 FALSE
#endif


#if VHAL_PWM
#define HAL_USE_PWM                 TRUE
#else
#define HAL_USE_PWM                 FALSE
#endif



#if VHAL_RTC
#define HAL_USE_RTC                 TRUE
#else
#define HAL_USE_RTC                 FALSE
#endif



#if VHAL_SDC
#define HAL_USE_SDC                 TRUE
#define SDC_INIT_RETRY              100
#define SDC_NICE_WAITING            TRUE
#define SDC_MMC_SUPPORT             FALSE
#else
#define HAL_USE_SDC                 FALSE
#endif


#if VHAL_SER
#define HAL_USE_SERIAL                 TRUE
#define HAL_USE_UART                   FALSE
#define SERIAL_DEFAULT_BITRATE      115200
#define SERIAL_BUFFERS_SIZE         128
#if VHAL_CDC
    #define HAL_USE_USB                 TRUE
    #define HAL_USE_SERIAL_USB          TRUE
#endif
#else
#define HAL_USE_SERIAL                 FALSE
#define HAL_USE_UART                   FALSE
#endif


#if VHAL_SPI
#define HAL_USE_SPI                 TRUE
#define SPI_USE_WAIT                TRUE
#define SPI_USE_MUTUAL_EXCLUSION    FALSE
#else
#define HAL_USE_SPI                 FALSE
#endif

#if VHAL_USB || VHAL_CDC
#define HAL_USE_USB                 TRUE
#else
#define HAL_USE_USB                 FALSE
#endif


#endif
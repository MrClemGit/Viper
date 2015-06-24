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

#include "ch.h"
#include "hal.h"

/**
 * @brief   PAL setup.
 * @details Digital I/O ports static configuration as defined in @p board.h.
 *          This variable is used by the HAL when initializing the PAL driver.
 */
#if HAL_USE_PAL || defined(__DOXYGEN__)
const PALConfig pal_default_config =
{
  {VAL_GPIOAODR, VAL_GPIOACRL, VAL_GPIOACRH},
  {VAL_GPIOBODR, VAL_GPIOBCRL, VAL_GPIOBCRH},
  {VAL_GPIOCODR, VAL_GPIOCCRL, VAL_GPIOCCRH},
  {VAL_GPIODODR, VAL_GPIODCRL, VAL_GPIODCRH},
  {VAL_GPIOEODR, VAL_GPIOECRL, VAL_GPIOECRH},
};
#endif

/*
 * Early initialization code.
 * This initialization must be performed just after stack setup and before
 * any other initialization.
 */
void __early_init(void) {

  stm32_clock_init();  
}

/*
 * Board-specific initialization code.
 */
void boardInit(void) {

  //need to reset and disable all interrupts left active by the bootloader...
  //https://github.com/spark/bootloader

  //disable EXTI2 interrupt
  EXTI->IMR   &= ~(1 << 2);
  EXTI->EMR   &= ~(1 << 2);
  EXTI->RTSR  &= ~(1 << 2);
  EXTI->FTSR  &= ~(1 << 2);
  EXTI->PR     =  (1 << 2);      
  NVIC_ClearPendingIRQ(EXTI2_IRQn);
  nvicDisableVector(EXTI2_IRQn);  

  //disable TIM1_CC  
  NVIC_ClearPendingIRQ(TIM1_CC_IRQn);
  nvicDisableVector(TIM1_CC_IRQn);

  //disable USB_CAN
  NVIC_ClearPendingIRQ(USB_LP_CAN1_RX0_IRQn);
  nvicDisableVector(USB_LP_CAN1_RX0_IRQn);

  //disable JTAG on D3-->D7
  //tim2/3 -> no mapping
  AFIO->MAPR = AFIO_MAPR_SWJ_CFG_DISABLE | AFIO_MAPR_TIM3_REMAP_NOREMAP  | AFIO_MAPR_TIM2_REMAP_NOREMAP| AFIO_MAPR_TIM1_REMAP_NOREMAP;
  
}


void systickhook(void){
}


/*
Core Pin out
============

  There are 24 pis on the Spark Core module.

  Spark     Spark Function                                         STM32F103CBT6
  Name      Pin #                           Pin #
  -------- ------ ------------------------------------------------ ---------------
   RAW     JP1-1  Input Power                                       N/A
   GND     JP1-2  GND
   A0     JP1-12  PA[00] WKUP/USART2_CTS/ADC12_IN0/TIM2_CH1_ETR     10
   A1     JP1-11  PA[01] USART2_RTS/ADC12_IN1/TIM2_CH2              11
   TX      JP1-3  PA[02] USART2_TX/ADC12_IN2/TIM2_CH3               12
   RX      JP1-4  PA[03] USART2_RX/ADC12_IN3/TIM2_CH4               13
   A2     JP1-10  PA[04] SPI1_NSS/USART2_CK/ADC12_IN4               14
   A3      JP1-9  PA[05] SPI1_SCK/ADC12_IN5                         15
   A4      JP1-8  PA[06] SPI1_MISO/ADC12_IN6/TIM3_CH1               16
   A5      JP1-7  PA[07] SPI1_MOSI/ADC12_IN7/TIM3_CH2               17
     LED2         PA[08] USART1_CK/TIM1_CH1/MCO                     29
     LED3         PA[09] USART1_TX/TIM1_CH2                         30
     LED4         PA[10] USART1_RX/TIM1_CH3                         31
   
     LED1,D7      PA[13] JTMS/SWDIO                                 34
   D7      JP2-5  PA[13] JTMS/SWDIO                                 34 Common with Blue LED LED_USR
   D6      JP2-6  PA[14] JTCK/SWCLK                                 37
   D5      JP2-7  PA[15] JTDI                                       38
   
  +3V3     JP2-1  V3.3 Out of Core                                  NA
   RST     JP2-2  NRST                                              7
   VDDA    JP2-3  ADC Voltage                                       9
   GND     JP2-4  GND
   
Core Internal IO
================

  Spark       Function                                          STM32F103CBT6
    Name                                    Pin #
  --------     ------------------------------------------------ ---------------
   A7      JP1-5  PB[01] ADC12_IN9/TIM3_CH4                         19
   A6      JP1-6  PB[00] ADC12_IN8/TIM3_CH3                         18
     BTN          PB[02] BOOT1                                      20
   D4      JP2-8  PB[03] JTDO                                       39
   D3      JP2-9  PB[04] NJTRST                                     40
   D2     JP2-10  PB[05] I2C1_SMBA                                  41
   D1     JP2-11  PB[06] I2C1_SCL/TIM4_CH1                          42
   D0     JP2-12  PB[07] I2C1_SDA/TIM4_CH2                          43
  WIFI_EN      PB[08] TIM4_CH3                                   45        CC3000 Module enable6
  MEM_CS       PB[09] TIM4_CH4                                   46       SST25VF016B Chip Select
  USB_DISC     PB[10] I2C2_SCL/USART3_TX                         21
  WIFI_INT     PB[11] I2C2_SDA/USART3_RX                         22        CC3000 Host interface SPI interrupt
  WIFI_CS      PB[12] SPI2_NSS/I2C2_SMBA/USART3_CK/TIM1_BKIN     25        CC3000 Chip Select
  SPI_CLK      PB[13] SPI2_SCK/USART3_CTS/TIM1_CH1N              26
  SPI_MISO     PB[14] SPI2_MISO/USART3_RTS/TIM1_CH2N             27
  SPI_MOSI     PB[15] SPI2_MOSI/TIM1_CH3N                        28

Buttons and LEDs
================

  Buttons
  -------
  The Spark has two mechanical buttons. One button is the RESET button
  connected to the STM32F103CB's reset line via /RST and the other is a
  generic user configurable button labeled BTN and connected to GPIO
  PB2/BOOT1. Since on the Spark, BOOT0 is tied to GND it is a moot point
  that BTN signal is connected to the BOOT1 signal. When a button is
  pressed it will drive the I/O line to GND.

  LEDs
  ----
  There are 4 user-controllable LEDs in two packages on board the Spark board:

      Sigal      Location     Color        GPIO    Active
      -------    ------------ -----------  -----  -----------
      LED1      LED_USR      Blue  LED    PA13    High  Common With D7
      LED2      LED_RGB      Red   LED    PA8     Low
      LED3      LED_RGB      Blue  LED    PA9     Low
      LED4      LED_RGB      Green LED    PA10    Low

  LED1 is connected to ground and can be illuminated by driving the PA13
  output high, it shares the Sparks D7 output. The LED2,LED3 and LED4
  are pulled high and can be illuminated by driving the corresponding GPIO output
  to low.  CCC(LED_TO(1,0,0));
  CCC(LED_OFF());
  CCC(LED_TO(1,0,0));
  CCC(LED_OFF());


  The RGB LEDs are not used by the board port unless CONFIG_ARCH_LEDS is
  defined.  In that case, the usage by the board port is defined in
  include/board.h and src/up_leds.c. The LEDs are used to encode OS-related
  events as follows:

      SYMBOL               Meaning                LED2    LED3   LED4
                                                  red     blue  green     Color
    ------------------- ----------------------- ------- ------- ------ ---------
    LED_STARTED         NuttX has been started  ON      OFF     OFF    Red
    LED_HEAPALLOCATE    Heap has been allocated OFF     ON      OFF    Blue
    LED_IRQSENABLED     Interrupts enabled      ON      OFF     ON     Orange
    LED_STACKCREATED    Idle stack created      OFF     OFF     ON     Green
    LED_INIRQ           In an interrupt**       ON      N/C     N/C    Orange Glow
    LED_SIGNAL          In a signal handler***  N/C     ON      N/C    Blue Glow
    LED_ASSERTION       An assertion failed     ON      ON      ON     White
    LED_PANIC           The system has crashed  ON      N/C     N/C    Red Flashing
    LED_IDLE            STM32 is is sleep mode  (Optional, not used)

    * If LED2, LED3, LED4 are statically on, then NuttX probably failed to boot
      and these LEDs will give you some indication of where the failure was
   ** The normal state is LED4 ON and LED2 faintly glowing.  This faint glow
      is because of timer interupts that result in the LED being illuminated
      on a small proportion of the time.
  *** LED3 may also flicker normally if signals are processed.

Serial Consoles
===============

  USART2
  -----
  If you have a 3.3 V TTL to RS-232 convertor then this is the most convenient
  serial console to use.  UART2 is the default in all of these
  configurations.

    USART2 RX  PA3   JP1 pin 4
    USART2 TX  PA2   JP1 pin 3
    GND             JP1 pin 2
    V3.3            JP2 pin 1

  Virtual COM Port
  ----------------
  Yet another option is to use UART0 and the USB virtual COM port.  This
  option may be more convenient for long term development, but was
  painful to use during board bring-up.
*/

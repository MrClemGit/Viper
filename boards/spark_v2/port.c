#include "port.h"
#include "vbl.h"
#include "lang.h"


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

*/



#define NUM_PINS   32
const uint16_t _vhalpinnums = NUM_PINS;

#define PA PORT_A
#define PB PORT_B
#define PC PORT_C
#define PD PORT_D

PinStatus _vhalpinstatus[NUM_PINS];

const PinInfo const _vhalpinmap[] STORED = {
    /* D0   */ MAKE_PIN(PB, 7,  HAS_EXT | HAS_ATM | HAS_I2C | HAS_SER),
    /* D1   */ MAKE_PIN(PB, 6,  HAS_EXT | HAS_ATM | HAS_I2C | HAS_SER),
    /* D2   */ MAKE_PIN(PB, 5, HAS_EXT | HAS_SPI),
    /* D3   */ MAKE_PIN(PB, 4, HAS_EXT | HAS_SPI),
    /* D4   */ MAKE_PIN(PB, 3, HAS_EXT | HAS_SPI),
    /* D5   */ MAKE_PIN(PA, 15, HAS_EXT),
    /* D6   */ MAKE_PIN(PA, 14, HAS_EXT),
    /* D7   */ MAKE_PIN(PA, 13, HAS_EXT),
    /* D8 RX   */ MAKE_PIN(PA, 3, HAS_EXT | HAS_ATM | HAS_ADC | HAS_SER),
    /* D9 TX   */ MAKE_PIN(PA, 2, HAS_EXT | HAS_ATM | HAS_ADC | HAS_SER),
    /* D10 BTN  */ MAKE_PIN(PB, 2, HAS_EXT),
    /* D11 WEN  */ MAKE_PIN(PB, 8, HAS_EXT),
    /* D12 WINT */ MAKE_PIN(PB, 11, HAS_EXT),
    /* D13 WCS  */ MAKE_PIN(PB, 12, HAS_EXT),
    /* D14 WCLK */ MAKE_PIN(PB, 13, HAS_EXT | HAS_SPI),
    /* D15 WMISO*/ MAKE_PIN(PB, 14, HAS_EXT | HAS_SPI),
    /* D16 WMOSI*/ MAKE_PIN(PB, 15, HAS_EXT | HAS_SPI),
    /* D17 LED0 */ MAKE_PIN(PA, 8, HAS_PWM),
    /* D18 LED1 */ MAKE_PIN(PA, 9, HAS_PWM),
    /* D19 LED2 */ MAKE_PIN(PA, 10, HAS_PWM),
    /* D20 MEM  */ MAKE_PIN(PB, 9, HAS_EXT),
    /* D21 USBD */ MAKE_PIN(PB, 10, HAS_EXT),
    /* D22 USBP */ MAKE_PIN(PA, 12, 0),
    /* D23 USBM */ MAKE_PIN(PA, 11, 0),
    /* A0   */ MAKE_PIN(PA, 0, HAS_EXT | HAS_ATM | HAS_ADC),
    /* A1   */ MAKE_PIN(PA, 1, HAS_EXT | HAS_ATM | HAS_ADC),
    /* A2   */ MAKE_PIN(PA, 4, HAS_EXT | HAS_ADC),
    /* A3   */ MAKE_PIN(PA, 5, HAS_EXT | HAS_ADC | HAS_SPI),
    /* A4   */ MAKE_PIN(PA, 6, HAS_EXT | HAS_ATM | HAS_ADC | HAS_SPI),
    /* A5   */ MAKE_PIN(PA, 7, HAS_EXT | HAS_ATM | HAS_ADC | HAS_SPI),
    /* A6   */ MAKE_PIN(PB, 0, HAS_EXT | HAS_ATM | HAS_ADC),
    /* A7   */ MAKE_PIN(PB, 1, HAS_EXT | HAS_ATM | HAS_ADC),

};


const PinClass const _analogclass[] STORED = {
    /* A0 */ MAKE_PIN_CLASS(24, 0, 0, 0),
    /* A1 */ MAKE_PIN_CLASS(25, 1, 0, 0),
    /* A2 */ MAKE_PIN_CLASS(26, 4, 0, 0),
    /* A3 */ MAKE_PIN_CLASS(27, 5, 0, 0),
    /* A4 */ MAKE_PIN_CLASS(28, 6, 0, 0),
    /* A5 */ MAKE_PIN_CLASS(29, 7, 0, 0),
    /* A6 */ MAKE_PIN_CLASS(30, 8, 0, 0),
    /* A7 */ MAKE_PIN_CLASS(31, 9, 0, 0),
};

const PinClass const _spiclass[] STORED = {
    /* MOSI0 */ MAKE_PIN_CLASS(2, 1, 0, 0),
    /* MISO0 */ MAKE_PIN_CLASS(3, 1, 0, 0),
    /* SCK0 */ MAKE_PIN_CLASS(4, 1, 0, 0),
    /* MOSI1 */ MAKE_PIN_CLASS(16, 2, 0, 0),
    /* MISO1 */ MAKE_PIN_CLASS(15, 2, 0, 0),
    /* SCK1 */ MAKE_PIN_CLASS(14, 2, 0, 0),

};

const PinClass const _i2cclass[] STORED = {
    /* SDA0 */ MAKE_PIN_CLASS(0, 1, 0, 0),
    /* SCL0 */ MAKE_PIN_CLASS(1, 1, 0, 0),
};


//phys, timer, channel
const PinClass const _pwmclass[] STORED = {
    /* PWM0 - D0 - */ MAKE_PIN_CLASS(0, 4, 2, 0),
    /* PWM1 - D1 + */ MAKE_PIN_CLASS(1, 4, 1, 0),
    /* PWM2 -RX  */ MAKE_PIN_CLASS(8, 2, 4, 0),
    /* PWM3 -TX */ MAKE_PIN_CLASS(9, 2, 3, 0),
    /* PWM4 -L0 */ MAKE_PIN_CLASS(17, 1, 1, 0),
    /* PWM5 -L1 */ MAKE_PIN_CLASS(18, 1, 2, 0),
    /* PWM6 -L2 */ MAKE_PIN_CLASS(19, 1, 3, 0),
    /* PWM7 - A0 -*/ MAKE_PIN_CLASS(24, 2, 1, 0),
    /* PWM8 - A1 +*/ MAKE_PIN_CLASS(25, 2, 2, 0),
    /* PWM9 - A4 -*/ MAKE_PIN_CLASS(28, 3, 1, 0),
    /* PWM10 - A5 -*/ MAKE_PIN_CLASS(29, 3, 2, 0),
    /* PWM11 -A6 -*/ MAKE_PIN_CLASS(30, 3, 3, 0),
    /* PWM12 -A7 +*/ MAKE_PIN_CLASS(31, 3, 4, 0),

};


const PinClass const _icuclass[] STORED = {
    /* PWM0 - D0 */ MAKE_PIN_CLASS(0, 4, 2, 0),
    /* PWM1 - D1 */ MAKE_PIN_CLASS(1, 4, 1, 0),
    /* PWM12 -RX */ MAKE_PIN_CLASS(8, 2, 4, 0),
    /* PWM13 -TX */ MAKE_PIN_CLASS(9, 2, 3, 0),
    /* PWM6 - A0 */ MAKE_PIN_CLASS(24, 2, 1, 0),
    /* PWM7 - A1 */ MAKE_PIN_CLASS(25, 2, 2, 0),
    /* PWM8 - A4 */ MAKE_PIN_CLASS(28, 3, 1, 0),
    /* PWM9 - A5 */ MAKE_PIN_CLASS(29, 3, 2, 0),
    /* PWM10 -A6 */ MAKE_PIN_CLASS(30, 3, 3, 0),
    /* PWM11 -A7 */ MAKE_PIN_CLASS(31, 3, 4, 0),
};

const PinClass const _canclass[] STORED = {};


/* phys, afio mask, afio val, afio shift  :: RM page 184*/
const PinClass const _serclass[] STORED = {
    /* RX0 1*/ MAKE_PIN_CLASS(0, 1, 1, 2),
    /* TX0 1*/ MAKE_PIN_CLASS(1, 1, 1, 2),
    /* RX1 2*/ MAKE_PIN_CLASS(8, 1, 0, 3),
    /* TX1 2*/ MAKE_PIN_CLASS(9, 1, 0, 3),
    /* RX2 usb*/ MAKE_PIN_CLASS(22, 0, 0, 0),
    /* TX2 usb*/ MAKE_PIN_CLASS(23, 0, 0, 0)

};

const PinClass const _dacclass[] STORED = {};

const PinClass const _ledclass[] STORED = {
    /* LED0 */ MAKE_PIN_CLASS(17, 0, 0, 0),
    /* LED1 */ MAKE_PIN_CLASS(18, 0, 0, 0),
    /* LED2 */ MAKE_PIN_CLASS(19, 0, 0, 0),
    /* LED3 */ MAKE_PIN_CLASS(7, 0, 0, 0)
};

const PinClass const _btnclass[] STORED = {
    /* BTN0 */ MAKE_PIN_CLASS(10, 0, 0, 0)
};



VHAL_PORT_DECLARATIONS();


/* PERIPHERAL MAPS */

BEGIN_PERIPHERAL_MAP(serial) \
PERIPHERAL_ID(4), \
PERIPHERAL_ID(2), \
PERIPHERAL_ID(1), \
END_PERIPHERAL_MAP(serial);


BEGIN_PERIPHERAL_MAP(spi) \
PERIPHERAL_ID(1), \
PERIPHERAL_ID(2), \
END_PERIPHERAL_MAP(spi);

BEGIN_PERIPHERAL_MAP(adc) \
PERIPHERAL_ID(1), \
END_PERIPHERAL_MAP(adc);


BEGIN_PERIPHERAL_MAP(pwm) \
PERIPHERAL_ID(1), \
PERIPHERAL_ID(2), \
PERIPHERAL_ID(3), \
PERIPHERAL_ID(4), \
END_PERIPHERAL_MAP(pwm);


BEGIN_PERIPHERAL_MAP(icu) \
PERIPHERAL_ID(2), \
PERIPHERAL_ID(3), \
PERIPHERAL_ID(4), \
END_PERIPHERAL_MAP(icu);


BEGIN_PERIPHERAL_MAP(htm) \
PERIPHERAL_ID(5), \
PERIPHERAL_ID(8), \
END_PERIPHERAL_MAP(htm);


/* vbl layer */

const SerialPins const _vm_serial_pins[] STORED = {
    {RX2, TX2},
    {RX1, TX1},
    {RX0, TX0},
};


void *begin_bytecode_storage(int size) {
    
    uint8_t *cm = codemem;
    vhalFlashErase(cm, size);
    return cm;
}

void *bytecode_store(void *where, uint8_t *buf, uint16_t len) {
    
    uint32_t bb = len - len % 4;

    if (where < (void *)0x8005000)
        return NULL;

    int ret = vhalFlashWrite(where, buf, bb);

    debug("bstored %i of %i\r\n", ret, bb);
    if ((uint32_t)ret != bb)
        return NULL;

    if (bb != len) {
        buf += bb;
        uint8_t bbuf[4];
        int i;
        for (i = 0; i < len % 4; i++) {
            bbuf[i] = buf[i];
        }
        ret = vhalFlashWrite( ((uint8_t *)where) + bb, bbuf, 4);
        if (ret != 4)
            return NULL;
        bb += 4;
    }

    return ((uint8_t *)where) + bb;
}

void *end_bytecode_storage() {
    return 0;
}

void *vbl_get_adjusted_codemem(void *codemem) {
    return vhalFlashAlignToSector(codemem);
}

/*   USB Layer   */


#if VHAL_CDC

#include "vhal_cdc.h"
#include "vhal_nfo.h"
/*
 * USB Device Descriptor.
 */
const uint8_t vcom_device_descriptor_data[18] = {
    USB_DESC_DEVICE       (0x0110,        /* bcdUSB (1.1).                    */
    0x02,          /* bDeviceClass (CDC).              */
    0x00,          /* bDeviceSubClass.                 */
    0x00,          /* bDeviceProtocol.                 */
    0x40,          /* bMaxPacketSize.                  */
    0x1d50,        /* idVendor (ST).                   */
    0x607D,        /* idProduct.                       */
    0x0200,        /* bcdDevice.                       */
    1,             /* iManufacturer.                   */
    2,             /* iProduct.                        */
    3,             /* iSerialNumber.                   */
    1)             /* bNumConfigurations.              */
};





/* Configuration Descriptor tree for a CDC.*/
const uint8_t vcom_configuration_descriptor_data[67] = {
    /* Configuration Descriptor.*/
    USB_DESC_CONFIGURATION(67,            /* wTotalLength.                    */
    0x02,          /* bNumInterfaces.                  */
    0x01,          /* bConfigurationValue.             */
    0,             /* iConfiguration.                  */
    0xC0,          /* bmAttributes (self powered).     */
    50),           /* bMaxPower (100mA).               */
    /* Interface Descriptor.*/
    USB_DESC_INTERFACE    (0x00,          /* bInterfaceNumber.                */
    0x00,          /* bAlternateSetting.               */
    0x01,          /* bNumEndpoints.                   */
    0x02,          /* bInterfaceClass (Communications
                                           Interface Class, CDC section
                                           4.2).                            */
    0x02,          /* bInterfaceSubClass (Abstract
                                         Control Model, CDC section 4.3).   */
    0x01,          /* bInterfaceProtocol (AT commands,
                                           CDC section 4.4).                */
    0),            /* iInterface.                      */
    /* Header Functional Descriptor (CDC section 5.2.3).*/
    USB_DESC_BYTE         (5),            /* bLength.                         */
    USB_DESC_BYTE         (0x24),         /* bDescriptorType (CS_INTERFACE).  */
    USB_DESC_BYTE         (0x00),         /* bDescriptorSubtype (Header
                                           Functional Descriptor.           */
    USB_DESC_BCD          (0x0110),       /* bcdCDC.                          */
    /* Call Management Functional Descriptor. */
    USB_DESC_BYTE         (5),            /* bFunctionLength.                 */
    USB_DESC_BYTE         (0x24),         /* bDescriptorType (CS_INTERFACE).  */
    USB_DESC_BYTE         (0x01),         /* bDescriptorSubtype (Call Management
                                           Functional Descriptor).          */
    USB_DESC_BYTE         (0x00),         /* bmCapabilities (D0+D1).          */
    USB_DESC_BYTE         (0x01),         /* bDataInterface.                  */
    /* ACM Functional Descriptor.*/
    USB_DESC_BYTE         (4),            /* bFunctionLength.                 */
    USB_DESC_BYTE         (0x24),         /* bDescriptorType (CS_INTERFACE).  */
    USB_DESC_BYTE         (0x02),         /* bDescriptorSubtype (Abstract
                                           Control Management Descriptor).  */
    USB_DESC_BYTE         (0x02),         /* bmCapabilities.                  */
    /* Union Functional Descriptor.*/
    USB_DESC_BYTE         (5),            /* bFunctionLength.                 */
    USB_DESC_BYTE         (0x24),         /* bDescriptorType (CS_INTERFACE).  */
    USB_DESC_BYTE         (0x06),         /* bDescriptorSubtype (Union
                                           Functional Descriptor).          */
    USB_DESC_BYTE         (0x00),         /* bMasterInterface (Communication
                                           Class Interface).                */
    USB_DESC_BYTE         (0x01),         /* bSlaveInterface0 (Data Class
                                           Interface).                      */
    /* Endpoint 2 Descriptor.*/
    USB_DESC_ENDPOINT     (USBD1_INTERRUPT_REQUEST_EP | 0x80,
    0x03,          /* bmAttributes (Interrupt).        */
    0x0008,        /* wMaxPacketSize.                  */
    0xFF),         /* bInterval.                       */
    /* Interface Descriptor.*/
    USB_DESC_INTERFACE    (0x01,          /* bInterfaceNumber.                */
    0x00,          /* bAlternateSetting.               */
    0x02,          /* bNumEndpoints.                   */
    0x0A,          /* bInterfaceClass (Data Class
                                           Interface, CDC section 4.5).     */
    0x00,          /* bInterfaceSubClass (CDC section
                                           4.6).                            */
    0x00,          /* bInterfaceProtocol (CDC section
                                           4.7).                            */
    0x00),         /* iInterface.                      */
    /* Endpoint 3 Descriptor.*/
    USB_DESC_ENDPOINT     (USBD1_DATA_AVAILABLE_EP,       /* bEndpointAddress.*/
    0x02,          /* bmAttributes (Bulk).             */
    0x0040,        /* wMaxPacketSize.                  */
    0x00),         /* bInterval.                       */
    /* Endpoint 1 Descriptor.*/
    USB_DESC_ENDPOINT     (USBD1_DATA_REQUEST_EP | 0x80,  /* bEndpointAddress.*/
    0x02,          /* bmAttributes (Bulk).             */
    0x0040,        /* wMaxPacketSize.                  */
    0x00)          /* bInterval.                       */
};


/*
 * U.S. English language identifier.
 */
const uint8_t vcom_string0[] = {
    USB_DESC_BYTE(4),                     /* bLength.                         */
    USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
    USB_DESC_WORD(0x0409)                 /* wLANGID (U.S. English).          */
};

/*
 * Vendor string.
 */
static const uint8_t vcom_string1[] = {
  USB_DESC_BYTE(18),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'P', 0, 'a', 0, 'r', 0, 't', 0, 'i', 0, 'c', 0, 'l', 0, 'e', 0
};

/*
 * Device Description string.
 */
static const uint8_t vcom_string2[] = {
  USB_DESC_BYTE(56),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'P', 0, 'a', 0, 'r', 0, 't', 0, 'i', 0, 'c', 0, 'l', 0, 'e', 0,
  ' ', 0, 'C', 0, 'o', 0, 'r', 0, 'e', 0, ' ', 0, 'V', 0, 'i', 0,
  'p', 0, 'e', 0, 'r', 0, 'i', 0, 'z', 0, 'e', 0, 'd', 0, ' ', 0,
  ' ', 0, ' ', 0, ' ', 0
};


/*
 * Serial Number string: filled in cdc with uid
 */
uint8_t vcom_string3[UID_BYTES*4+2];


/*
 * Configuration Descriptor wrapper.
 */
const USBDescriptor vcom_configuration_descriptor = {
    sizeof vcom_configuration_descriptor_data,
    vcom_configuration_descriptor_data
};

/*
 * Strings wrappers array.
 */
const USBDescriptor vcom_strings[] = {
    {sizeof vcom_string0, vcom_string0},
    {sizeof vcom_string1, vcom_string1},
    {sizeof vcom_string2, vcom_string2},
    {sizeof vcom_string3, vcom_string3}
};

/*
 * Device Descriptor wrapper.
 */
const USBDescriptor vcom_device_descriptor = {
    sizeof vcom_device_descriptor_data,
    vcom_device_descriptor_data
};

#endif

#include "vbl.h"
#include "port.h"
#include "lang.h"


#define NUM_PINS   77

#define PA PORT_A
#define PB PORT_B
#define PC PORT_C
#define PD PORT_D

PinStatus _vhalpinstatus[NUM_PINS];

const PinInfo const _vhalpinmap[] STORED = {
    /* D0   */ MAKE_PIN(PA, 8,  HAS_EXT | HAS_SER),
    /* D1   */ MAKE_PIN(PA, 9,  HAS_EXT | HAS_SER),
    /* D2   */ MAKE_PIN(PB, 25,  HAS_EXT | HAS_ICU), /* TIOA0 = channel 0   B */
    /* D3   */ MAKE_PIN(PC, 28,  HAS_EXT | HAS_ICU), /* TIOA7 = channel 14 B */
    /* D4   */ MAKE_PIN(PA, 29,  HAS_EXT), /* TIOB6 = channel 13 B */
    /* D5   */ MAKE_PIN(PC, 25,  HAS_EXT | HAS_ICU), /* TIOA6 = channel 12 B */
    /* D6   */ MAKE_PIN(PC, 24,  HAS_EXT | HAS_PWM), /* PWML7 */
    /* D7   */ MAKE_PIN(PC, 23,  HAS_EXT | HAS_PWM), /* PWML6 */
    /* D8   */ MAKE_PIN(PC, 22,  HAS_EXT | HAS_PWM), /* PWML5 */
    /* D9   */ MAKE_PIN(PC, 21,  HAS_EXT | HAS_PWM), /* PWML4 */
    /* D10  */ MAKE_PIN(PA, 28,  HAS_EXT), /* TIOB7 = channel 15 B */
    /* D11  */ MAKE_PIN(PD, 7,  HAS_EXT | HAS_ICU),  /* TIOA8 = channel 16 B */
    /* D12  */ MAKE_PIN(PD, 8,  HAS_EXT),  /* TIOB8 = channel 17 B */
    /* D13  */ MAKE_PIN(PB, 27,  HAS_EXT), /* TIOB0 = channel 1 B */
    /* D14  */ MAKE_PIN(PD, 4,  HAS_EXT),
    /* D15  */ MAKE_PIN(PD, 5,  HAS_EXT),
    /* D16  */ MAKE_PIN(PA, 13,  HAS_EXT | HAS_PWM),
    /* D17  */ MAKE_PIN(PA, 12,  HAS_EXT | HAS_PWM),
    /* D18  */ MAKE_PIN(PA, 11,  HAS_EXT),
    /* D19  */ MAKE_PIN(PA, 10,  HAS_EXT),
    /* D20  */ MAKE_PIN(PB, 12,  HAS_EXT | HAS_I2C | HAS_PWM),
    /* D21  */ MAKE_PIN(PB, 13,  HAS_EXT | HAS_I2C | HAS_PWM),
    /* D22  */ MAKE_PIN(PB, 26,  HAS_EXT),
    /* D23  */ MAKE_PIN(PA, 14,  HAS_EXT),
    /* D24  */ MAKE_PIN(PA, 15,  HAS_EXT),
    /* D25  */ MAKE_PIN(PD, 0,  HAS_EXT),
    /* D26  */ MAKE_PIN(PD, 1,  HAS_EXT),
    /* D27  */ MAKE_PIN(PD, 2,  HAS_EXT),
    /* D28  */ MAKE_PIN(PD, 3,  HAS_EXT),
    /* D29  */ MAKE_PIN(PD, 6,  HAS_EXT),
    /* D30  */ MAKE_PIN(PD, 9,  HAS_EXT),
    /* D31  */ MAKE_PIN(PA, 7,  HAS_EXT),
    /* D32  */ MAKE_PIN(PD, 10,  HAS_EXT),
    /* D33  */ MAKE_PIN(PC, 1,  HAS_EXT),
    /* D34  */ MAKE_PIN(PC, 2,  HAS_EXT | HAS_PWM),
    /* D35  */ MAKE_PIN(PC, 3,  HAS_EXT | HAS_PWM),
    /* D36  */ MAKE_PIN(PC, 4,  HAS_EXT | HAS_PWM),
    /* D37  */ MAKE_PIN(PC, 5,  HAS_EXT | HAS_PWM),
    /* D38  */ MAKE_PIN(PC, 6,  HAS_EXT | HAS_PWM),
    /* D39  */ MAKE_PIN(PC, 7,  HAS_EXT | HAS_PWM),
    /* D40  */ MAKE_PIN(PC, 8,  HAS_EXT | HAS_PWM),
    /* D41  */ MAKE_PIN(PC, 9,  HAS_EXT | HAS_PWM),
    /* D42  */ MAKE_PIN(PA, 19,  HAS_EXT | HAS_PWM),
    /* D43  */ MAKE_PIN(PA, 20,  HAS_EXT | HAS_PWM),
    /* D44  */ MAKE_PIN(PC, 19,  HAS_EXT | HAS_PWM),
    /* D45  */ MAKE_PIN(PC, 18,  HAS_EXT | HAS_PWM),
    /* D46  */ MAKE_PIN(PC, 17,  HAS_EXT),
    /* D47  */ MAKE_PIN(PC, 16,  HAS_EXT),
    /* D48  */ MAKE_PIN(PC, 15,  HAS_EXT),
    /* D49  */ MAKE_PIN(PC, 14,  HAS_EXT),
    /* D50  */ MAKE_PIN(PC, 13,  HAS_EXT),
    /* D51  */ MAKE_PIN(PC, 12,  HAS_EXT),
    /* D52  */ MAKE_PIN(PB, 21,  HAS_EXT | HAS_SER),
    /* D53  */ MAKE_PIN(PB, 14,  HAS_EXT),
    /* D54  */ MAKE_PIN(PB, 15,  HAS_EXT | HAS_DAC),
    /* D55  */ MAKE_PIN(PB, 16,  HAS_EXT | HAS_DAC),
    /* D56  */ MAKE_PIN(PA, 1,  HAS_EXT | HAS_CAN),
    /* D57  */ MAKE_PIN(PA, 0,  HAS_EXT | HAS_CAN),
    /* D58  */ MAKE_PIN(PA, 17,  HAS_EXT | HAS_I2C),
    /* D59  */ MAKE_PIN(PA, 18,  HAS_EXT | HAS_I2C),
    /* D60  */ MAKE_PIN(PC, 30,  HAS_EXT),
    /* D61  */ MAKE_PIN(PA, 21,  HAS_EXT),
    /* D62  */ MAKE_PIN(PA, 25,  HAS_EXT | HAS_SPI),
    /* D63  */ MAKE_PIN(PA, 26,  HAS_EXT | HAS_SPI),
    /* D64  */ MAKE_PIN(PA, 27,  HAS_EXT | HAS_SPI),

    /* A0  */ MAKE_PIN(PA, 16,  HAS_EXT | HAS_ADC),
    /* A1  */ MAKE_PIN(PA, 24,  HAS_EXT | HAS_ADC),
    /* A2  */ MAKE_PIN(PA, 23,  HAS_EXT | HAS_ADC),
    /* A3  */ MAKE_PIN(PA, 22,  HAS_EXT | HAS_ADC),
    /* A4  */ MAKE_PIN(PA, 6,  HAS_EXT | HAS_ADC),
    /* A5  */ MAKE_PIN(PA, 4,  HAS_EXT | HAS_ADC),
    /* A6  */ MAKE_PIN(PA, 3,  HAS_EXT | HAS_ADC),
    /* A7  */ MAKE_PIN(PA, 2,  HAS_EXT | HAS_ADC),
    /* A8  */ MAKE_PIN(PB, 17,  HAS_EXT | HAS_ADC),
    /* A9  */ MAKE_PIN(PB, 18,  HAS_EXT | HAS_ADC),
    /* A10  */ MAKE_PIN(PB, 19,  HAS_EXT | HAS_ADC),
    /* A11  */ MAKE_PIN(PB, 20,  HAS_EXT | HAS_ADC | HAS_SER),

};


const PinClass const _analogclass[] STORED = {
    /* A0 */ MAKE_PIN_CLASS(65, 7 , 0, 0),
    /* A1 */ MAKE_PIN_CLASS(66, 6 , 0, 0),
    /* A2 */ MAKE_PIN_CLASS(67, 5 , 0, 0),
    /* A3 */ MAKE_PIN_CLASS(68, 4 , 0, 0),
    /* A4 */ MAKE_PIN_CLASS(69, 3 , 0, 0),
    /* A5 */ MAKE_PIN_CLASS(70, 2, 0, 0),
    /* A6 */ MAKE_PIN_CLASS(71, 1, 0, 0),
    /* A7 */ MAKE_PIN_CLASS(72, 0, 0, 0),
    /* A8 */ MAKE_PIN_CLASS(73, 10, 0, 0),
    /* A9 */ MAKE_PIN_CLASS(74, 11, 0, 0),
    /* A10 */ MAKE_PIN_CLASS(75, 12, 0, 0),
    /* A11 */ MAKE_PIN_CLASS(76, 13, 0, 0),
};

const PinClass const _spiclass[] STORED = {
    /* MOSI0 */ MAKE_PIN_CLASS(63, 1, 0, 0),
    /* MISO0 */ MAKE_PIN_CLASS(62, 1, 0, 0),
    /* SCK0 */ MAKE_PIN_CLASS(64, 1, 0, 0),
};

const PinClass const _i2cclass[] STORED = {
    /* SDA0 */ MAKE_PIN_CLASS(20, 1, 0, 0),
    /* SCL0 */ MAKE_PIN_CLASS(21, 1, 0, 0),
    /* SDA1 */ MAKE_PIN_CLASS(58, 2, 0, 0),
    /* SCL1 */ MAKE_PIN_CLASS(59, 2, 0, 0),
};

//OLD
//DATA0: if TIO -> 0, if PWM -> 1
//DATA1: if TIO -> index of ch (0..18   2*x+(1 if B 0 if A))      if PWM -> idx of pwm
//DATA3: 0 = PAD_A  1= PAD_B

// NEW
// DATA0 -> PWM Channel
// DATA1 -> PORT Id
// DATA2 -> 0 = H, 1 = L

const PinClass const _pwmclass[] STORED = {
    /* D0 */ MAKE_PIN_CLASS(0, 0, 0, 0),
    /* D1 */ MAKE_PIN_CLASS(1, 3, 0, 0),
    /* D6 */ MAKE_PIN_CLASS(6, 7, 2, 1),
    /* D7 */ MAKE_PIN_CLASS(7, 6, 2, 1),
    /* D8 */ MAKE_PIN_CLASS(8, 5, 2, 1),
    /* D9 */ MAKE_PIN_CLASS(9, 4, 2, 1),
    /* D16 */ MAKE_PIN_CLASS(16, 2, 0, 0),
    /* D17 */ MAKE_PIN_CLASS(17, 1, 0, 1),
    /* D20 */ MAKE_PIN_CLASS(20, 0, 1, 0),
    /* D21 */ MAKE_PIN_CLASS(21, 1, 1, 0),
    /* D34 */ MAKE_PIN_CLASS(34, 0, 2, 1),
    /* D35 */ MAKE_PIN_CLASS(35, 0, 2, 0),
    /* D36 */ MAKE_PIN_CLASS(36, 1, 2, 1),
    /* D37 */ MAKE_PIN_CLASS(37, 1, 2, 0),
    /* D38 */ MAKE_PIN_CLASS(38, 2, 2, 1),
    /* D39 */ MAKE_PIN_CLASS(39, 2, 2, 0),
    /* D40 */ MAKE_PIN_CLASS(40, 3, 2, 1),
    /* D41 */ MAKE_PIN_CLASS(41, 3, 2, 0),
    /* D42 */ MAKE_PIN_CLASS(42, 1, 0, 0),
    /* D43 */ MAKE_PIN_CLASS(43, 2, 0, 1),
    /* D44 */ MAKE_PIN_CLASS(43, 5, 2, 0),
    /* D45 */ MAKE_PIN_CLASS(43, 6, 2, 0),
    /* D53 */ MAKE_PIN_CLASS(53, 2, 1, 0),
    /* D62 */ MAKE_PIN_CLASS(62, 1, 1, 1),
    /* D63 */ MAKE_PIN_CLASS(63, 2, 1, 1),
    /* D64 */ MAKE_PIN_CLASS(64, 3, 1, 1),
    /* D66 */ MAKE_PIN_CLASS(66, 3, 1, 0),
    /* D67 */ MAKE_PIN_CLASS(67, 0, 1, 1),
};

// DATA0 -> TC Channel
// DATA1 -> PIO Peripheral (A = 0, B = 1)

const PinClass const _icuclass[] STORED = {
    /* PIN2    */ MAKE_PIN_CLASS(2, 0, 1, 0),
    /* PIN3    */ MAKE_PIN_CLASS(3, 7, 1, 0),
    /* PIN5    */ MAKE_PIN_CLASS(5, 6, 1, 0),
    /* PIN11   */ MAKE_PIN_CLASS(11, 8, 1, 0),
    /* A7 (72) */ MAKE_PIN_CLASS(72, 1, 0, 0),
};

const PinClass const _canclass[] STORED = {
    /*CANRX*/ MAKE_PIN_CLASS(68, 0, 0, 0),
    /*CANTX*/ MAKE_PIN_CLASS(69, 0, 0, 0),
};

const PinClass const _serclass[] STORED = {
    /* RX0 */ MAKE_PIN_CLASS(0, 4, 0, 1),
    /* TX0 */ MAKE_PIN_CLASS(1, 4, 0, 1),
    /* RX1 */ MAKE_PIN_CLASS(19, 0, 0, 1),
    /* TX1 */ MAKE_PIN_CLASS(18, 0, 0, 1),
    /* RX2 */ MAKE_PIN_CLASS(17, 1, 0, 1),
    /* TX2 */ MAKE_PIN_CLASS(16, 1, 0, 1),
    /* RX3 */ MAKE_PIN_CLASS(15, 3, 0, 2),
    /* TX3 */ MAKE_PIN_CLASS(14, 3, 0, 2),
    /* RX4 */ MAKE_PIN_CLASS(52, 2, 0, 1),
    /* TX4 */ MAKE_PIN_CLASS(65, 2, 0, 1),
};

const PinClass const _dacclass[] STORED = {
    /*DAC0*/ MAKE_PIN_CLASS(66, 0, 0, 0),
    /*DAC1*/ MAKE_PIN_CLASS(67, 0, 0, 0),

};

const PinClass const _ledclass[] STORED = {
    /* LED0 */ MAKE_PIN_CLASS(13, 0, 0, 0),
    /* LED1 */ MAKE_PIN_CLASS(60, 0, 0, 0),
    /* LED2 */ MAKE_PIN_CLASS(61, 0, 0, 0),
};

const PinClass const _btnclass[] STORED = {
};


VHAL_PORT_DECLARATIONS();


/* PERIPHERAL MAPS */

/*     USART0 USART1 USART2 USART3 UART */
/*code    1      2      3      4     5  */
/*map     1      2      4      3     0  */
BEGIN_PERIPHERAL_MAP(serial) \
PERIPHERAL_ID(5), \
PERIPHERAL_ID(1), \
PERIPHERAL_ID(2), \
PERIPHERAL_ID(4), \
PERIPHERAL_ID(3), \
END_PERIPHERAL_MAP(serial);

const SerialPins const _vm_serial_pins[] STORED = {
    {RX0, TX0},
    {RX1, TX1},
    {RX2, TX2},
    {RX3, TX3},
    {RX4, TX4},
};

BEGIN_PERIPHERAL_MAP(spi) \
PERIPHERAL_ID(1), \
END_PERIPHERAL_MAP(spi);

BEGIN_PERIPHERAL_MAP(adc) \
PERIPHERAL_ID(1), \
END_PERIPHERAL_MAP(adc);


BEGIN_PERIPHERAL_MAP(dac) \
PERIPHERAL_ID(1), \
PERIPHERAL_ID(2), \
END_PERIPHERAL_MAP(dac);

BEGIN_PERIPHERAL_MAP(can) \
PERIPHERAL_ID(1), \
END_PERIPHERAL_MAP(can);



BEGIN_PERIPHERAL_MAP(pwm) \
PERIPHERAL_ID(1), \
PERIPHERAL_ID(2), \
PERIPHERAL_ID(3), \
PERIPHERAL_ID(4), \
PERIPHERAL_ID(5), \
PERIPHERAL_ID(6), \
PERIPHERAL_ID(7), \
PERIPHERAL_ID(8), \
PERIPHERAL_ID(9), \
PERIPHERAL_ID(10), \
PERIPHERAL_ID(11), \
PERIPHERAL_ID(12), \
END_PERIPHERAL_MAP(pwm);


BEGIN_PERIPHERAL_MAP(icu) \
PERIPHERAL_ID(1), \
PERIPHERAL_ID(2), \
PERIPHERAL_ID(3), \
PERIPHERAL_ID(4), \
PERIPHERAL_ID(5), \
END_PERIPHERAL_MAP(icu);


BEGIN_PERIPHERAL_MAP(htm) \
PERIPHERAL_ID(1), \
PERIPHERAL_ID(2), \
PERIPHERAL_ID(3), \
PERIPHERAL_ID(4), \
PERIPHERAL_ID(5), \
PERIPHERAL_ID(6), \
PERIPHERAL_ID(7), \
PERIPHERAL_ID(8), \
PERIPHERAL_ID(9), \
END_PERIPHERAL_MAP(htm);

/* vbl layer */


void *begin_bytecode_storage(int size) {
    //address of the second bank
    if (codemem < 0xC0000)
        return (uint8_t *)0xC0000;
    else
        return codemem;
}

void *bytecode_store(void *where, uint8_t *buf, uint16_t len) {
    return flash_write_buffer((uint32_t *)where, buf, len);
}

void *end_bytecode_storage() {
    return 0;
}

void *vbl_get_adjusted_codemem(void *codemem) {
    if (codemem < 0xC0000)
        return (uint8_t *)0xC0000;
    else
        return codemem;
}
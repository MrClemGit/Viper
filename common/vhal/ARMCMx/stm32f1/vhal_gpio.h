#ifndef __VHAL_GPIO__
#define __VHAL_GPIO__


#define GPIO_PORT ioportid_t
#define CPIN_PORT(vpin) (GPIO_PORT)PIN_PORT(vpin)

#define vPinSetModeEx(port,pad,mode) palSetPadMode(port,pad,mode)
#define vPinSetMode(port,pad,mode) palSetPadMode(port,pad,_pinmodes[mode])
#define vPinRead(port,pad) palReadPad(port,pad)
#define vPinWrite(port,pad,val) palWritePad(port,pad,val)


#define PAL_MODE_INPUT                  2
#define PAL_MODE_INPUT_PULLUP           3
#define PAL_MODE_INPUT_PULLDOWN         4


#define STM32_MODE_ANALOG           5

#define STM32_OTYPE_PUSHPULL        6
#define STM32_OTYPE_OPENDRAIN       7
#define STM32_OTYPE_ALTERNATE_PUSHPULL   16
#define STM32_OTYPE_ALTERNATE_OPENDRAIN  17

#define STM32_PUDR_NOPULL           2
#define STM32_PUDR_PULLUP           3
#define STM32_PUDR_PULLDOWN         4


#define STM32_ALTERNATE_MASK        (0xfff0)
#define STM32_ALTERNATE_VAL_MASK    (0xff00)
#define STM32_ALTERNATE_SHIFT_MASK  (0x00f0)
#define STM32_ALTERNATE(val,shift)  (((val)<<16)|((shift)<<8))

#define ALTERNATE_FN(val,shift) STM32_ALTERNATE(val,shift)

#define PINPRM_CNF_ANALOG 0
#define PINPRM_CNF_FLOATING 1
#define PINPRM_CNF_INPUT 2

#define PINPRM_CNF_GPIO_PP 0
#define PINPRM_CNF_GPIO_OD 1
#define PINPRM_CNF_ALTERNATE_PP 2
#define PINPRM_CNF_ALTERNATE_OD 3

#define PINPRM_MODE_INPUT 0
#define PINPRM_MODE_10MHZ 1
#define PINPRM_MODE_2MHZ 2
#define PINPRM_MODE_50MHZ 3


#define PIN_PARAMS(mode,cnf,shift,mask,val) ((((mode)|((cnf)<<2))&0xff)| (((shift)<<8)&0xff00) | (((mask)<<16)&0xff0000) | (((val)<<24)&0xff000000))


extern const uint8_t _pinmodes[];

#endif
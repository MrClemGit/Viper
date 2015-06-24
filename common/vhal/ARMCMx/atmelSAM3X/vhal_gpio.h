#ifndef __VHAL_GPIO__
#define __VHAL_GPIO__


#define GPIO_PORT ioportid_t
#define CPIN_PORT(vpin) (GPIO_PORT)PIN_PORT(vpin)

#define vPinSetModeEx(port,pad,mode) palSetPadMode(port,pad,mode)
#define vPinSetMode(port,pad,mode) palSetPadMode(port,pad,_pinmodes[mode])
#define vPinRead(port,pad) palReadPad(port,pad)
#define vPinWrite(port,pad,val) palWritePad(port,pad,val)


#define SAM3X_PERIPHERAL_A    (0x10)
#define SAM3X_PERIPHERAL_B    (0x20)

//port.c must pass 1==A 2==B in PIN_CLASS_DATA2
#define SAM3X_PAD(x) ((x)<<4)
#define SAM3X_PIN_PR(vpin) SAM3X_PAD(PIN_CLASS_DATA2(vpin))

extern const uint8_t _pinmodes[];

#endif
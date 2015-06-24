Viper Hardware Abstraction Layer
================================

VHAL is responsible for the opaque abstraction of the underlying mcu peripherals.
Every VHAL function returns an int. In the resul is >=0 then the function executed correctly, otherwise it's an error code.

### GPIO pins

Different boards route different pins to different places in the board layout. Also their naming convention differs. In this scenarion, the VHAL introduces the concept of pin *class*, pin *pad* and *virtual pin*.
A virtual pin is a 16 bit word that contains the pin class and pin pad info. A virtual pin is represented in con with a *v* before the variable name (vpin, vpin0, vpin1...). A physical pin *x* may correspond to more than one virtual pin, if *x* has many different alternate functions. For example a physical pin that can be used as analog or digital, is indexed by two different virtual pins: the one with class *analog* and the other with class *digital*. 

The abstraction of virtual pin is used throughout the VHAL, every vhal function takes as input a virtual pin. The correspondence between physical and virtual pin is a responsibility of the board port and is defined in boards/BOARD/port.c. A constant array of PinInfo structures is defined as _vhalpinmap and contains the description of physical pins: PORT, PAD, and an or'ed list of FLAGS specifying the peripherals it supports. Virtual pins are then created by filling the *class* arrays (always in port.c), where PinClass structures contain, for every class, an index pointing to the physical pin in _vhalpinmap, and three byte data fields that can be used differently by different architecture. In the stm32f4 version of the VHAL they contain peripheral and channel specification for PWM class, or usart number for serial class, etc...

The VHAL provides a bunch of macro to navigate the mapping:
* PIN_CLASS(vpin): get the class id of the virtual pin
* PIN_CLASS_PAD(vpin): get the index of the vpin inside the corresponding class array
* PIN_CLASS_ID(vpin): get the index of the vpin inside the physical pins array, _vhalpinmap
* PIN_CLASS_DATAn(vpin): for n in [0:2], gets the virtual pin data
* PIN_PORT_NUMBER(vpin): gets the number of the port of the physical pin pointed by the vpin
* PIN_PORT(vpin): gets the pointer to the GPIO register of the physical pin pointed by the vpin
* PIN_PAD(vpin): gets the pad number of the physical pin pointed by the vpin
* PIN_PRPH(vpin): gets the flags describing the peripherals attached to the physical pin pointed by the vpin
* PIN_HAS_PRPH(vpin, prph): cheks if the physical pin pointed by the vpin supports the peripherals prph (a list of flags can be found in vhal_pins.h in common/inc)
* CPIN_PORT(vpin): this macro is defined in vhal_gpio.h, a file specific of every single VHAL port. It is the same of PIN_PORT(vpin) except that the resul is casted to the correct type excpected by the low level routines handling pins (i.e. in stm32f4, the cast is down towards ioportid_t, because the low level routines used are the chibios ones)

Pin modes are described in vhal.h. Alternate functions for a pin are used only inside a VHAL port, the user of the VHAL is not allowed to change the function of a pin with vhalPinSetMode because alternate function setting is highly platform dependent. If an alternate function is needed, it is implemented in the corresponding driver.


### Peripherals

In VHAL also peripherals are *virtual*. They are enumerated by a virtual index from 0 onwards and such index will be passed to all vhal functions requiring a peripheral specification. The correspondence between a virtual peripheral index and a physical peripheral is again described in port.c. Each board declares a set of arrays containing the index of the physical peripheral at the array position of the virtual peripheral. An example can clarify: stm32f4 chips support up to 8 serials, but the stm32f401 (used in the st nucleo) only supports 3 three of them, specifically the usart1, usart2 and usart6. Therefore in the port.c for the st_nucleo a _vhal_serial_map array is defined containing the following `{2,6,1}`, meaning that serial 0 will be mapped to usart2, serial 1 will be mapped to usart6 and serial 2 will be mapped to usart1. _vhal_serial_map is exported so that the stm32f4 VHAL implementation can know how many serial drivers to implement. Refer to vhal_ser.c in stm32f4 for an example implementation. To facilitate the navigation of virtual peripherals the following macro are provided:
* PERIPHERAL_ID(n): returns the physical id of a peripheral. For example PERIPHERAL_ID(6) in vhal_ser.c return an index used to identify the usart6. Always refer to physical peripherals via this macro
* GET_PERIPHERAL_ID(type,vprph): gets the physical id of the virtual peripheral *vprph* of peripheral type *type*. For example GET_PERIPHERAL_ID(serial,0) corresponds to PERIPHERAL_ID(2) in the st_nucleo board port.
* PERIPHERAL_NUM(type): gets the number of peripherals for *type*.
* BEGIN_PERIPHERAL_MAP(type): is used in port.c to declare the peripheral mapping array for the peripheral of type *type*. Following this macro, a comma separated list of PERIPHERAL_ID(n) macro must be inserted.
* END_PERIPHERAL_MAP(type): ends a peripheral mapping.


### External Interrupts

Handling of GPIO external interrupts in VHAL is made by using a single function: vhalPinAttachInterrupt(vpin,mode,fn). *mode* for external interrupts is defined in vhal.h and specifies falling or rising edge, or both. The function *fn* will be called when the interrupt fires and the channel (or slot) of the interrupt, together with the direction of the signal (falling, rising) are passed. To disable an external interrupt, pass NULL for fn.
vhalPinAttachInterrupt returns the *slot* the interrupt has been attached to.

### PWM

Handling of PWM is made by using a single function: vhalPwmStart(vpin,period,pulse). The vpin is set in pwm mode with a period and a pulse. Time units in VHAL are generally expressed by the TIME_U(t,u) macro: the least significant bit of t is sacrified to hold the time unit u, being it microseconds or milliseconds. vhalPwmStart takes such times for period and pulse. For example, the call vhalPwmStart(D10,TIME_U(1,MILLIS),TIME_U(400,MICROS)) starts the PWM peripherals (if supported) on pin D10 with a period of 1 millisecond and a pulse of 400 microseconds. The resulting square wave is high for 400 microseconds and low for 600 microseconds, with a frequency of 1kHz (1000 periods in a second). To disable PWM, pass a period of 0.
When writing a PWM driver for VHAL, care must be taken because often PWM peripherals are actually timer peripherals with capture/compare capabilities. Therefore, even if the VHAL separates PWM from ICU from Hardware Timers, they could be all sharing the some hardware.


### ICU

Input capture units are usually implemented by Hardware timer peripherals. An ICU works like a timer connected to an external interrupt. Everytime an interrupt is generated, the time passed since the last interrupt is read from the timer and stored somewhere. At the end of the process we have a complete description of the train of interrupts. In VHAL the ICU units are handled with a single function: vhalIcuStart(vpin, conf). The parameter conf is a structure holding:
* the minimum sample time to capture (min_sample): it is used to determine the best capture frequency to set the hardware timer or icu unit.
* the maximum sample time to capture (max_sample): it is used to determine the best capture frequency to set the hardware timer or icu unit. If during a capture, max_sample is elapsed without a capture, the capture stops.
* trigger: selects the starting condition for the ICU unit, being it a transition from high to low or viceversa
* mode: selects the modality of data reporting. ICU unit can be configured to report only the duration of a pulse (low to high to low, high to low to high), or the duration of a period (time between two transitions with the same direction: low to high to low to high), or time between any transition
* filter: specifies what fraction of the min_sample time must pass before the signal is acknowledged as an event. Values are from 0..15 and express the percentage of min_sample
* cfg: trigger, mode and filter are compressed in a single value, *cfg* by means of the macro ICU_CFG(mode,trigger,filter)
* buffer: is a uint32_t pointer pointing to memory where the values of the current capture are stored
* bufsize: is the maximum size of the buffer
* callback: is a function called at every capture taking as parameters the direction of the event (fall or rise), the time passed (encoded with TIME_U), and the position where it has been stored in buffer (if buffer is not NULL). The callback, knowing the current capture position in the buffer must return a flag to reset the buffer (and start again from the beginning) or keep on going with the capture or stop the capture. If the callback doesn't reset the capture at the end of the buffer, the capture stops. This way a continuous capture mechanism can be implemented, or the capture can be stopped when the needed data is received. If no callback is given, the capture stops at the end of the buffer.

vhalIcuStart will return the number of captures stored in the buffer or an error.


```
     __________________         ______
     |                 |        |    |
     |                 |        |    |
_____|                 |________|    |_____

     <-----PULSE------>
     <-----PERIOD-------------->
     <-----TIME1------><-TIME2->
```

### HTM (Hardware timers)


How to write a VHAL driver
==========================

### General Tips & Hints
1. Respect the conventions in vhal.h! Functions names must start with "vhal" immediately followed by camel-cased vhal peripheral name and function name. Example: vhalSerialWrite, vhalRngGenerate...
2. Try to write all the code in a single .c file even if the driver implements more than one vhal peripheral.
3. If the driver must use an ISR, follow the convention naming in vhal_vectors.h. And, if thread-safeness is needed, lock the rtos via the vosal layer as soon as the ISR is entered. Remember to unlock!
4. If the driver must control the clock of a peripheral, enable the clock in the init function and disable the clock in the done function for power saving (unless the clock is shared by multiple drivers). Reset and clock functions are defined and implemented in the vendor library (inside inc/ and src/ directories of the vhal specific implementation).
5. If the peripheral needs to control some pins, remember to assign such pins to the peripheral in the init function of the driver and to release them when done. Pin assignment to peripherals is usually described in datasheets (DS_* documents inside the docs/ directory of the vhal specific implementation). If different sets of pins can be assigned to the same peripheral either the pins are passed to the init function, either the init function implements only a fixed set of assignable pins (board dependent).
6. If the peripheral needs some setup at system initialization, refer to vhal.c for the correct function to implement. Such vhalInit* functions are declared as weak, therefore, if not implemented, they are linked to a standard implementation that does nothing.


### Example
TBD

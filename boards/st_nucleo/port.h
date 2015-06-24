#ifndef __PLATFORM_CONFIG__
#define __PLATFORM_CONFIG__


#define PORT_PROVIDE_RANDOM FALSE
#define UID_BYTES   12

/*====== DRIVER MAPPING ================================================== */


#define EXT_INTERRUPTS_NEEDED   16
#define EXT_SLOTS EXT_INTERRUPTS_NEEDED


/*====== PORT HOOKS ================================================ */

#define PORT_PRE_UPLOAD_HOOK()
#define PORT_AFTER_UPLOAD_HOOK() vosThSleep(TIME_U(500,MILLIS));
#define PORT_LED_CONFIG()\
    vhalPinSetMode(LED0,PINMODE_OUTPUT_PUSHPULL);  \
    vhalPinWrite(LED0,0)
#define PORT_LED_ON() vhalPinWrite(LED0,1)
#define PORT_LED_OFF() vhalPinWrite(LED0,0)


#endif

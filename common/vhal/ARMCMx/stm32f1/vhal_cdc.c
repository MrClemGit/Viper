#include "vhal_cdc.h"
#include "vhal_nfo.h"

#if VHAL_CDC

SerialUSBDriver SDU1;





/*
 * Serial over USB Driver structure.
 */


extern const USBDescriptor vcom_configuration_descriptor;
extern const USBDescriptor vcom_device_descriptor;
extern const USBDescriptor vcom_strings[];


void __fill_serial_usb_id(uint8_t *desc){

	desc[0] = USB_DESC_BYTE(2+4*UID_BYTES);
    desc[1] = USB_DESC_BYTE(USB_DESCRIPTOR_STRING);
    int i,j;
    for(i=0;i<UID_BYTES;i++){
    	desc[2+i*4+1]=0;
    	desc[2+i*4+3]=0;
    	j=(UID_ADDR[i]&0x0f);
    	desc[2+i*4]= (j<10) ? (j+'0'):(j+'A'-10);
    	j=((UID_ADDR[i]&0xf0)>>4);
    	desc[2+i*4+2]=(j<10) ? (j+'0'):(j+'A'-10);
    }
}

int vhalInitCDC(void *data){
	(void)data;
	__fill_serial_usb_id((uint8_t*) vcom_strings[3].ud_string);
	return 0;
}

/*
 * Handles the GET_DESCRIPTOR callback. All required descriptors must be
 * handled here.
 */
static const USBDescriptor *get_descriptor(USBDriver *usbp,
        uint8_t dtype,
        uint8_t dindex,
        uint16_t lang) {

    (void)usbp;
    (void)lang;
    switch (dtype) {
        case USB_DESCRIPTOR_DEVICE:
            return &vcom_device_descriptor;
        case USB_DESCRIPTOR_CONFIGURATION:
            return &vcom_configuration_descriptor;
        case USB_DESCRIPTOR_STRING:
            if (dindex < 4)
                return &vcom_strings[dindex];
    }
    return NULL;
}

/**
 * @brief   IN EP1 state.
 */
static USBInEndpointState ep1instate;

/**
 * @brief   OUT EP1 state.
 */
static USBOutEndpointState ep1outstate;

/**
 * @brief   EP1 initialization structure (both IN and OUT).
 */
static const USBEndpointConfig ep1config = {
    USB_EP_MODE_TYPE_BULK,
    NULL,
    sduDataTransmitted,
    sduDataReceived,
    0x0040,
    0x0040,
    &ep1instate,
    &ep1outstate,
    1,
    NULL
};


/**
 * @brief   IN EP2 state.
 */
static USBInEndpointState ep2instate;

/**
 * @brief   EP2 initialization structure (IN only).
 */
static const USBEndpointConfig ep2config = {
    USB_EP_MODE_TYPE_INTR,
    NULL,
    sduInterruptTransmitted,
    NULL,
    0x0010,
    0x0000,
    &ep2instate,
    NULL,
    1,
    NULL
};

/*
 * Handles the USB driver global events.
 */
static void usb_event(USBDriver *usbp, usbevent_t event) {

    switch (event) {
        case USB_EVENT_RESET:
            return;
        case USB_EVENT_ADDRESS:
            return;
        case USB_EVENT_CONFIGURED:
            chSysLockFromIsr();

            /* Enables the endpoints specified into the configuration.
               Note, this callback is invoked from an ISR so I-Class functions
               must be used.*/
            usbInitEndpointI(usbp, USBD1_DATA_REQUEST_EP, &ep1config);
            usbInitEndpointI(usbp, USBD1_INTERRUPT_REQUEST_EP, &ep2config);

            /* Resetting the state of the CDC subsystem.*/
            sduConfigureHookI(&SDU1);

            chSysUnlockFromIsr();
            return;
        case USB_EVENT_SUSPEND:
            return;
        case USB_EVENT_WAKEUP:
            return;
        case USB_EVENT_STALLED:
            return;
    }
    return;
}

/*
 * USB driver configuration.
 */
const USBConfig vhal_usbcfg = {
    usb_event,
    get_descriptor,
    sduRequestsHook,
    NULL
};

/*
 * Serial over USB driver configuration.
 */
const SerialUSBConfig vhal_serusbcfg = {
    &USBD1,
    USBD1_DATA_REQUEST_EP,
    USBD1_DATA_AVAILABLE_EP,
    USBD1_INTERRUPT_REQUEST_EP
};



#endif
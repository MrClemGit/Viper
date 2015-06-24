#ifndef __PLATFORM_CONFIG__
#define __PLATFORM_CONFIG__


#define PORT_PROVIDE_RANDOM FALSE
#define UID_BYTES   12

/*====== DRIVER MAPPING ================================================== */


#define EXT_INTERRUPTS_NEEDED   16
#define EXT_SLOTS EXT_INTERRUPTS_NEEDED


/*====== PORT HOOKS ================================================ */

#define PORT_PRE_UPLOAD_HOOK() vosThSleep(TIME_U(5000,MILLIS));
#define PORT_AFTER_UPLOAD_HOOK() vosThSleep(TIME_U(1000,MILLIS));
#define PORT_LED_CONFIG()\
    vhalPinSetMode(LED0,PINMODE_OUTPUT_PUSHPULL);  \
    vhalPinSetMode(LED1,PINMODE_OUTPUT_PUSHPULL);  \
    vhalPinSetMode(LED2,PINMODE_OUTPUT_PUSHPULL);  \
    vhalPinWrite(LED0,1); \
    vhalPinWrite(LED1,1); \
    vhalPinWrite(LED2,1)
#define PORT_LED_ON() vhalPinWrite(LED0,0)
#define PORT_LED_OFF() vhalPinWrite(LED0,1)


#endif





/*

#define ADC_SAMPLE_RATE(pin,rate)  ((rate)<<(3*(pin)))
#define ADC_SELECTED_CHANNEL(pin) ADC_SQR3_SQ1_N(pin)
#define ADC_NUM_CHANNELS(num) ADC_SQR1_NUM_CH(num)

#define DRIVER_ADC_CONFIG(cfg, samplerate, resolution) (cfg)->dummy = ADC_SAMPLE_55P5

#define DRIVER_ADC_GROUP_SINGLE(cfg,drvcfg,pin)\
    (cfg)->circular =FALSE;\
    (cfg)->num_channels =1;\
    (cfg)->end_cb =NULL;\
    (cfg)->error_cb =NULL;\
    (cfg)->cr1 =0;\
    (cfg)->cr2 =0;\
    (cfg)->smpr1 =0;\
    (cfg)->smpr2 = ADC_SAMPLE_RATE(PIN_DATA(pin),(drvcfg)->dummy);\
    (cfg)->sqr1=ADC_NUM_CHANNELS(1);\
    (cfg)->sqr2=0;\
    (cfg)->sqr3=ADC_SELECTED_CHANNEL(PIN_DATA(pin))





#define OAL_SERIAL1_DRIVER  &SD2
#define OAL_SERIAL_USB1_DRIVER  &SDU1


#define DRIVER_SERIAL_CONFIG(cfg,baud,parity,stop,bits) \
    (cfg)->speed = baud; \
    (cfg)->cr1 = 0; \
    (cfg)->cr2 = (USART_CR2_STOP1_BITS | USART_CR2_LINEN)

#define DRIVER_SERIAL_PARITY_NO      0
#define DRIVER_SERIAL_PARITY_EVEN    USART_CR1_PCE
#define DRIVER_SERIAL_PARITY_ODD     (USART_CR1_PCE  | USART_CR1_PS)
#define DRIVER_SERIAL_STOP_1       0
#define DRIVER_SERIAL_STOP_2       (USART_CR2_STOP_1)
#define DRIVER_SERIAL_CHAR_8_BIT   1
#define DRIVER_SERIAL_CHAR_9_BIT   0




#if OAL_PWM

#define PWM_CALLBACKS 10
#define PWM_DRV_SIZE 3

#define PWM_DECLARATIONS() \
extern PWMConfig _drv_pwm_cfg1; \
extern PWMConfig _drv_pwm_cfg2; \
extern PWMConfig _drv_pwm_cfg3 

#define PWM_DRV_CONFIGS {&_drv_pwm_cfg1,&_drv_pwm_cfg2,&_drv_pwm_cfg3}
#define PWM_DRV_DRIVERS {&PWMD2, &PWMD3, &PWMD4}


extern int32_t _pwm_cycles[PWM_CALLBACKS]; //A4 A5 A6 A7 A0 A1 A9 A8 D1 D0


#define PWM_CALLBACK_INIT(pin_pwm,pin_pad,cycles)\
    if (pin_pwm == 1) _pwm_cycles[pin_pad] = cycles;\
    else if (pin_pwm == 0) _pwm_cycles[4 + pin_pad] = cycles;\
    else _pwm_cycles[8 + pin_pad] = cycles


#endif



#if OAL_SPI1 || OAL_SPI2

#define SPI_Direction_2Lines_FullDuplex ((uint16_t)0x0000)
#define SPI_Direction_2Lines_RxOnly     ((uint16_t)0x0400)
#define SPI_Direction_1Line_Rx          ((uint16_t)0x8000)
#define SPI_Direction_1Line_Tx          ((uint16_t)0xC000)
#define IS_SPI_DIRECTION_MODE(MODE) (((MODE) == SPI_Direction_2Lines_FullDuplex) || \
                                     ((MODE) == SPI_Direction_2Lines_RxOnly) || \
                                     ((MODE) == SPI_Direction_1Line_Rx) || \
                                     ((MODE) == SPI_Direction_1Line_Tx))

#define SPI_Mode_Master                 ((uint16_t)0x0104)
#define SPI_Mode_Slave                  ((uint16_t)0x0000)
#define IS_SPI_MODE(MODE) (((MODE) == SPI_Mode_Master) || \
                           ((MODE) == SPI_Mode_Slave))

#define SPI_DataSize_16b                ((uint16_t)0x0800)
#define SPI_DataSize_8b                 ((uint16_t)0x0000)
#define IS_SPI_DATASIZE(DATASIZE) (((DATASIZE) == SPI_DataSize_16b) || \
                                   ((DATASIZE) == SPI_DataSize_8b))

#define SPI_CPOL_Low                    ((uint16_t)0x0000)
#define SPI_CPOL_High                   ((uint16_t)0x0002)
#define IS_SPI_CPOL(CPOL) (((CPOL) == SPI_CPOL_Low) || \
                           ((CPOL) == SPI_CPOL_High))

#define SPI_CPHA_1Edge                  ((uint16_t)0x0000)
#define SPI_CPHA_2Edge                  ((uint16_t)0x0001)
#define IS_SPI_CPHA(CPHA) (((CPHA) == SPI_CPHA_1Edge) || \
                           ((CPHA) == SPI_CPHA_2Edge))

#define SPI_NSS_Soft                    ((uint16_t)0x0200)
#define SPI_NSS_Hard                    ((uint16_t)0x0000)
#define IS_SPI_NSS(NSS) (((NSS) == SPI_NSS_Soft) || \
                         ((NSS) == SPI_NSS_Hard))

#define SPI_BaudRatePrescaler_2         ((uint16_t)0x0000)
#define SPI_BaudRatePrescaler_4         ((uint16_t)0x0008)
#define SPI_BaudRatePrescaler_8         ((uint16_t)0x0010)
#define SPI_BaudRatePrescaler_16        ((uint16_t)0x0018)
#define SPI_BaudRatePrescaler_32        ((uint16_t)0x0020)
#define SPI_BaudRatePrescaler_64        ((uint16_t)0x0028)
#define SPI_BaudRatePrescaler_128       ((uint16_t)0x0030)
#define SPI_BaudRatePrescaler_256       ((uint16_t)0x0038)
#define IS_SPI_BAUDRATE_PRESCALER(PRESCALER) (((PRESCALER) == SPI_BaudRatePrescaler_2) || \
        ((PRESCALER) == SPI_BaudRatePrescaler_4) || \
        ((PRESCALER) == SPI_BaudRatePrescaler_8) || \
        ((PRESCALER) == SPI_BaudRatePrescaler_16) || \
        ((PRESCALER) == SPI_BaudRatePrescaler_32) || \
        ((PRESCALER) == SPI_BaudRatePrescaler_64) || \
        ((PRESCALER) == SPI_BaudRatePrescaler_128) || \
        ((PRESCALER) == SPI_BaudRatePrescaler_256))

#define SPI_FirstBit_MSB                ((uint16_t)0x0000)
#define SPI_FirstBit_LSB                ((uint16_t)0x0080)
#define IS_SPI_FIRST_BIT(BIT) (((BIT) == SPI_FirstBit_MSB) || \
                               ((BIT) == SPI_FirstBit_LSB))

#endif


*/

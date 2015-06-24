#ifndef __VHAL_TIM__
#define __VHAL_TIM__

#include "vhal_common.h"

#define FALSE 0
#define TRUE  (!FALSE)

/*******************  Definition for TC_CCRx register (see doc page 880) ********************/
typedef union {
	struct {
		uint32_t CLKEN 		: 1;	/*!<Counter Clock Enable Command        */
		uint32_t CLKDIS 	: 1;	/*!<Counter Clock Disable Command       */
		uint32_t SWTRG 		: 1;	/*!<Software Trigger Command            */
	} fields;
	uint32_t	value;
} TC_CCR_TypeDef;

/*******************  Definition for TC_CMRx register (see doc page 881) ********************/
typedef union {
	union {
        /* TC_CMRx Fields in Capture Mode */
        struct {
            uint32_t TCCLKS     : 3;    /*!<Clock Selection                     */
            uint32_t CLKI       : 1;    /*!<Clock Invert                        */
            uint32_t BURST      : 2;    /*!<Burst Signal Selection              */
            uint32_t LDBSTOP    : 1;
            uint32_t LDBDIS     : 1;
            uint32_t ETRGEDG    : 2;
            uint32_t ABETRG     : 1;
            uint32_t RESERVED0  : 3;
            uint32_t CPCTRG     : 1;
            uint32_t WAVE       : 1;
            uint32_t LDRA       : 2;
            uint32_t LDRB       : 2;
        } capture;

        /* TC_CMRx Fields in Waveform Mode */
        struct {
            uint32_t TCCLKS     : 3;    /*!<Clock Selection                     */
            uint32_t CLKI       : 1;    /*!<Clock Invert                        */
            uint32_t BURST      : 2;    /*!<Burst Signal Selection              */
            uint32_t CPCSTOP    : 1;
            uint32_t CPCDIS     : 1;
            uint32_t EEVTEDG    : 2;
            uint32_t EEVT       : 1;
            uint32_t ENTRG      : 1;
            uint32_t WAVSEL     : 2;
            uint32_t WAVE       : 1;
            uint32_t ACPA       : 2;
            uint32_t ACPC       : 2;
            uint32_t AEEVT      : 2;
            uint32_t ASWTRG     : 2;
            uint32_t BCPB       : 2;
            uint32_t BCPC       : 2;
            uint32_t BEEVT      : 2;
            uint32_t BSWTRG     : 2;
        } waveform;
    } fields;

    /* Register Value */
	uint32_t	value;
} TC_CMR_TypeDef;

/* Possible values for TC_CMR_TCCLKS */
// MCK / 2
#define  TC_CMR_TIMER_CLOCK1                0
// MCK / 8
#define  TC_CMR_TIMER_CLOCK2                1
// MCK / 32
#define  TC_CMR_TIMER_CLOCK3                2
// MCK / 128
#define  TC_CMR_TIMER_CLOCK4                3
// SLCK (Slow Clock)
#define  TC_CMR_TIMER_CLOCK5                4
#define  TC_CMR_XC0                         5
#define  TC_CMR_XC1                         6
#define  TC_CMR_XC2                         7


/*******************  Definition for TC_SMMRx register (see doc page 887) ********************/
typedef union {
    struct {
        uint32_t GCEN       : 1;    /*!<Gray Count Enabled        */
        uint32_t DOWN       : 1;    /*!<Down Count      */
    } fields;
    uint32_t    value;
} TC_SMMR_TypeDef;


/*******************  Definition for TC_SRx register (see doc page 892) ********************/
typedef union {
    struct {
        uint32_t COVFS      : 1;    /*!<Counter Overflow        */
        uint32_t LOVRS      : 1;    /*!<Load Overrun Status      */
        uint32_t CPAS       : 1;    /*!<RA Compare Status      */
        uint32_t CPBS       : 1;
        uint32_t CPCS       : 1;
        uint32_t LDRAS      : 1;
        uint32_t LDRBS      : 1;
        uint32_t ETRGS      : 1;
        uint32_t RESERVED0  : 8;
        uint32_t CLKSTA     : 1;
        uint32_t MTIOA      : 1;
        uint32_t MTIOB      : 1;
    } fields;
    uint32_t    value;
} TC_SR_TypeDef;


/*******************  Definition for TC_IERx register (see doc page 894) ********************/
typedef union {
    struct {
        uint32_t COVFS      : 1;    /*!<Counter Overflow        */
        uint32_t LOVRS      : 1;    /*!<Load Overrun Status      */
        uint32_t CPAS       : 1;    /*!<RA Compare Status      */
        uint32_t CPBS       : 1;
        uint32_t CPCS       : 1;
        uint32_t LDRAS      : 1;
        uint32_t LDRBS      : 1;
        uint32_t ETRGS      : 1;
    } fields;
    uint32_t    value;
} TC_IER_TypeDef;


/*******************  Definition for TC_IDRx register (see doc page 894) ********************/
typedef union {
    struct {
        uint32_t COVFS      : 1;    /*!<Counter Overflow        */
        uint32_t LOVRS      : 1;    /*!<Load Overrun Status      */
        uint32_t CPAS       : 1;    /*!<RA Compare Status      */
        uint32_t CPBS       : 1;
        uint32_t CPCS       : 1;
        uint32_t LDRAS      : 1;
        uint32_t LDRBS      : 1;
        uint32_t ETRGS      : 1;
    } fields;
    uint32_t    value;
} TC_IDR_TypeDef;


/*******************  Definition for TC_IDRx register (see doc page 894) ********************/
typedef union {
    struct {
        uint32_t COVFS      : 1;    /*!<Counter Overflow        */
        uint32_t LOVRS      : 1;    /*!<Load Overrun Status      */
        uint32_t CPAS       : 1;    /*!<RA Compare Status      */
        uint32_t CPBS       : 1;
        uint32_t CPCS       : 1;
        uint32_t LDRAS      : 1;
        uint32_t LDRBS      : 1;
        uint32_t ETRGS      : 1;
    } fields;
    uint32_t    value;
} TC_IMR_TypeDef;


typedef struct {
    __O         TC_CCR_TypeDef       TC_CCR;			/*!< TC Channel Control Register */
    __IO        TC_CMR_TypeDef       TC_CMR;			/*!< TC Channel Mode Register: Capture Mode */
    __IO        TC_SMMR_TypeDef      TC_SMMR;
                uint32_t             RESERVED0;
    __O         uint32_t             TC_CV;
    __IO        uint32_t             TC_RA;
    __IO        uint32_t             TC_RB;
    __IO        uint32_t             TC_RC;
    __O         TC_SR_TypeDef        TC_SR;
    __I         TC_IER_TypeDef       TC_IER;
    __I         TC_IDR_TypeDef       TC_IDR;
    __O         TC_IMR_TypeDef       TC_IMR;
} TIM_CHAN_TypeDef;


/*******************  Definition for TC_BCR register (see doc page 900) ********************/
typedef union {
    struct {
        uint32_t SYNC       : 1;    /*!<Synchro Command        */
    } fields;
    uint32_t    value;
} TC_BCR_TypeDef;


/*******************  Definition for TC_BMR register (see doc page 901) ********************/
typedef union {
    struct {
        uint32_t TC0XC0S    : 2;
        uint32_t TC1XC1S    : 2;
        uint32_t TC2XC2S    : 2;
        uint32_t RESERVED0  : 2;
        uint32_t QDEN       : 1;
        uint32_t POSEN      : 1;
        uint32_t SPEEDEN    : 1;
        uint32_t QDTRANS    : 1;
        uint32_t EDGPHA     : 1;
        uint32_t INVA       : 1;
        uint32_t INVB       : 1;
        uint32_t INVIDX     : 1;
        uint32_t SWAP       : 1;
        uint32_t IDXPHB     : 1;
        uint32_t RESERVED1  : 2;
        uint32_t MAXFILT    : 6;
    } fields;
    uint32_t    value;
} TC_BMR_TypeDef;


/*******************  Definition for TC_QIER register (see doc page 903) ********************/
typedef union {
    struct {
        uint32_t IDX        : 1;
        uint32_t DIRCHG     : 1;
        uint32_t QERR       : 1;
    } fields;
    uint32_t    value;
} TC_QIER_TypeDef;


/*******************  Definition for TC_QIER register (see doc page 903) ********************/
typedef union {
    struct {
        uint32_t IDX        : 1;
        uint32_t DIRCHG     : 1;
        uint32_t QERR       : 1;
    } fields;
    uint32_t    value;
} TC_QIDR_TypeDef;


/*******************  Definition for TC_QIMR register (see doc page 903) ********************/
typedef union {
    struct {
        uint32_t IDX        : 1;
        uint32_t DIRCHG     : 1;
        uint32_t QERR       : 1;
    } fields;
    uint32_t    value;
} TC_QIMR_TypeDef;


/*******************  Definition for TC_QIMR register (see doc page 903) ********************/
typedef union {
    struct {
        uint32_t IDX        : 1;
        uint32_t DIRCHG     : 1;
        uint32_t QERR       : 1;
    } fields;
    uint32_t    value;
} TC_QISR_TypeDef;


/*******************  Definition for TC_FMR register (see doc page 907) ********************/
typedef union {
    struct {
        uint32_t ENCF0      : 1;
        uint32_t ENCF1      : 1;
    } fields;
    uint32_t    value;
} TC_FMR_TypeDef;


/*******************  Definition for TC_WPMR register (see doc page 908) ********************/
typedef union {
    struct {
        uint32_t WPEN       : 1;
        uint32_t RESERVED0  : 7;
        uint32_t WPKEY      : 24;
    } fields;
    uint32_t    value;
} TC_WPMR_TypeDef;


typedef struct {
                TIM_CHAN_TypeDef	channels[3];
    __I	        TC_BCR_TypeDef          TC_BCR;
    __IO        TC_BMR_TypeDef          TC_BMR;
    __O         TC_QIER_TypeDef 	TC_QIER;
    __O         TC_QIDR_TypeDef 	TC_QIDR;
    __I         TC_QIMR_TypeDef 	TC_QIMR;
    __I         TC_QISR_TypeDef         TC_QISR;
    __IO        TC_FMR_TypeDef          TC_FMR;
    __IO        TC_WPMR_TypeDef         TC_WPMR;
                uint8_t                 RESERVED0[20];
} TIM_TypeDef;


/* The total number of Timer Counter on SAM3X8E */
#define		TIMNUM				3
/* The total number of Timer Channels per counter (see doc page 856) */
#define		CHANNUM				3

/* The actual timers of SAM3X8E */
#define  MODULE_TIM1                         ((TIM_TypeDef *) (0x40080000))
#define  MODULE_TIM2                         ((TIM_TypeDef *) (0x40084000))
#define  MODULE_TIM3                         ((TIM_TypeDef *) (0x40088000))

/* Each channel is mapped on a virtual timer */
#define TIM1                                 ((MODULE_TIM1)->channels[0])
#define TIM2                                 ((MODULE_TIM1)->channels[1])
#define TIM3                                 ((MODULE_TIM1)->channels[2])
#define TIM4                                 ((MODULE_TIM2)->channels[0])
#define TIM5                                 ((MODULE_TIM2)->channels[1])
#define TIM6                                 ((MODULE_TIM2)->channels[2])
#define TIM7                                 ((MODULE_TIM3)->channels[0])
#define TIM8                                 ((MODULE_TIM3)->channels[1])
#define TIM9                                 ((MODULE_TIM3)->channels[2])


/* PWM Clock Register */
typedef union {
    struct {
        uint32_t DIVA       : 8;
        uint32_t PREA       : 4;
        uint32_t RESERVED0  : 4;
        uint32_t DIVB       : 8;
        uint32_t PREB       : 4;
        uint32_t RESERVED1  : 4;
    } fields;
    uint32_t    value;
} PWM_CLK_TypeDef;

/* PWM Enable Register */
typedef union {
    struct {
        uint32_t CHID0      : 1;
        uint32_t CHID1      : 1;
        uint32_t CHID2      : 1;
        uint32_t CHID3      : 1;
        uint32_t CHID4      : 1;
        uint32_t CHID5      : 1;
        uint32_t CHID6      : 1;
        uint32_t CHID7      : 1;
        uint32_t RESERVED0  : 24;
    } fields;
    uint32_t    value;
} PWM_ENA_TypeDef;

/* PWM Write Protect Control Register */
typedef union {
    struct {
        uint32_t WPCMD      : 2;
        uint32_t WPRG0      : 1;
        uint32_t WPRG1      : 1;
        uint32_t WPRG2      : 1;
        uint32_t WPRG3      : 1;
        uint32_t WPRG4      : 1;
        uint32_t WPRG5      : 1;
        uint32_t WPKEY      : 24;
    } fields;
    uint32_t    value;
} PWM_WPCR_TypeDef;

/* PWM Channel Mode Register */
typedef union {
    struct {
        uint32_t CPRE       : 4;
        uint32_t RESERVED0  : 4;
        uint32_t CALG       : 1;
        uint32_t CPOL       : 1;
        uint32_t CES        : 1;
        uint32_t RESERVED1  : 5;
        uint32_t DTE        : 1;
        uint32_t DTHI       : 1;
        uint32_t DTLI       : 1;
        uint32_t RESERVED2  : 13;
    } fields;
    uint32_t    value;
} PWM_CMR_TypeDef;

/* PWM Channel Period Register */
typedef union {
    struct {
        uint32_t CPRD       : 24;
        uint32_t RESERVED0  : 8;
    } fields;
    uint32_t    value;
} PWM_CPRD_TypeDef;

/* PWM Duty Cycle Register */
typedef union {
    struct {
        uint32_t CDTY       : 24;
        uint32_t RESERVED0  : 8;
    } fields;
    uint32_t    value;
} PWM_CDTY_TypeDef;



#define  REGISTER_PWM_CLK                    ((PWM_CLK_TypeDef *) (0x40094000))
#define  REGISTER_PWM_ENA                    ((PWM_ENA_TypeDef *) (0x40094004))
#define  REGISTER_PWM_WPCR                   ((PWM_WPCR_TypeDef *) (0x400940E4))

// PWM CHANNEL MODE Registers
#define  REGISTER_PWM_CMR0                   ((PWM_CMR_TypeDef *) (0x40094200))
#define  REGISTER_PWM_CMR1                   ((PWM_CMR_TypeDef *) (0x40094220))
#define  REGISTER_PWM_CMR2                   ((PWM_CMR_TypeDef *) (0x40094240))
#define  REGISTER_PWM_CMR3                   ((PWM_CMR_TypeDef *) (0x40094260))
#define  REGISTER_PWM_CMR4                   ((PWM_CMR_TypeDef *) (0x40094280))
#define  REGISTER_PWM_CMR5                   ((PWM_CMR_TypeDef *) (0x400942A0))
#define  REGISTER_PWM_CMR6                   ((PWM_CMR_TypeDef *) (0x400942C0))
#define  REGISTER_PWM_CMR7                   ((PWM_CMR_TypeDef *) (0x400942E0))

// PWM Period Registers
#define  REGISTER_PWM_CPRD0                  ((PWM_CPRD_TypeDef *) (0x4009420C))
#define  REGISTER_PWM_CPRD1                  ((PWM_CPRD_TypeDef *) (0x4009422C))
#define  REGISTER_PWM_CPRD2                  ((PWM_CPRD_TypeDef *) (0x4009424C))
#define  REGISTER_PWM_CPRD3                  ((PWM_CPRD_TypeDef *) (0x4009426C))
#define  REGISTER_PWM_CPRD4                  ((PWM_CPRD_TypeDef *) (0x4009428C))
#define  REGISTER_PWM_CPRD5                  ((PWM_CPRD_TypeDef *) (0x400942AC))
#define  REGISTER_PWM_CPRD6                  ((PWM_CPRD_TypeDef *) (0x400942CC))
#define  REGISTER_PWM_CPRD7                  ((PWM_CPRD_TypeDef *) (0x400942EC))

// PWM Duty Cycle Registers
#define  REGISTER_PWM_CDTY0                  ((PWM_CDTY_TypeDef *) (0x40094204))
#define  REGISTER_PWM_CDTY1                  ((PWM_CDTY_TypeDef *) (0x40094224))
#define  REGISTER_PWM_CDTY2                  ((PWM_CDTY_TypeDef *) (0x40094244))
#define  REGISTER_PWM_CDTY3                  ((PWM_CDTY_TypeDef *) (0x40094264))
#define  REGISTER_PWM_CDTY4                  ((PWM_CDTY_TypeDef *) (0x40094284))
#define  REGISTER_PWM_CDTY5                  ((PWM_CDTY_TypeDef *) (0x400942A4))
#define  REGISTER_PWM_CDTY6                  ((PWM_CDTY_TypeDef *) (0x400942C4))
#define  REGISTER_PWM_CDTY7                  ((PWM_CDTY_TypeDef *) (0x400942E4))


#define PWM_CHANNEL_IS_ACTIVE(channel)       ((PWM->PWM_SR & 0x1 << channel) != 0)


/*
int vhalInitTIM(void *data);
int vhalHtmGetFreeTimer(void);
int vhalHtmOneShot(uint32_t tm, uint32_t delay, htmFn fn, void *args);
int vhalHtmRecurrent(uint32_t tm, uint32_t delay, htmFn fn, void *args);
int vhalPwmStart(int vpin, uint32_t period, uint32_t pulse);
int vhalIcuStart(int vpin, vhalIcuConf *conf);
*/
#endif
/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    templates/adc_lld.h
 * @brief   ADC Driver subsystem low level driver header template.
 *
 * @addtogroup ADC
 * @{
 */

#ifndef _ADC_LLD_H_
#define _ADC_LLD_H_

#if HAL_USE_ADC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   ADC1 driver enable switch.
 * @details If set to @p TRUE the support for ADC1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(SAM3X_ADC_USE_ADC1) || defined(__DOXYGEN__)
#define SAM3X_ADC_USE_ADC1               FALSE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

typedef Adc *adcdef_t;

/**
 * @brief   ADC sample data type.
 */
typedef uint16_t adcsample_t;

/**
 * @brief   Channels number in a conversion group.
 */
typedef uint16_t adc_channels_num_t;

/**
 * @brief   Possible ADC failure causes.
 * @note    Error codes are architecture dependent and should not relied
 *          upon.
 */
typedef enum {
    ADC_ERR_DMAFAILURE = 0,                   /**< DMA operations failure.    */
    ADC_ERR_OVERFLOW = 1                      /**< ADC overflow condition.    */
} adcerror_t;

/**
 * @brief   Type of a structure representing an ADC driver.
 */
typedef struct ADCDriver ADCDriver;

/**
 * @brief   ADC notification callback type.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object triggering the
 *                      callback
 * @param[in] buffer    pointer to the most recent samples data
 * @param[in] n         number of buffer rows available starting from @p buffer
 */
typedef void (*adccallback_t)(ADCDriver *adcp, adcsample_t *buffer, size_t n);

/**
 * @brief   ADC error callback type.
 *
 * @param[in] adcp      pointer to the @p ADCDriver object triggering the
 *                      callback
 * @param[in] err       ADC error code
 */
typedef void (*adcerrorcallback_t)(ADCDriver *adcp, adcerror_t err);

/**
 * @brief   Conversion group configuration structure.
 * @details This implementation-dependent structure describes a conversion
 *          operation.
 * @note    Implementations may extend this structure to contain more,
 *          architecture dependent, fields.
 */
typedef struct {
    /**
     * @brief   Enables the circular buffer mode for the group.
     */
    bool_t                    circular;
    /**
     * @brief   Number of the analog channels belonging to the conversion group.
     */
    adc_channels_num_t        num_channels;
    /**
     * @brief   Callback function associated to the group or @p NULL.
     */
    adccallback_t             end_cb;
    /**
     * @brief   Error callback or @p NULL.
     */
    adcerrorcallback_t        error_cb;
    /* End of the mandatory fields.*/
    uint32_t channels;
    uint32_t seq1;
    uint32_t seq2;
} ADCConversionGroup;

/**
 * @brief   Driver configuration structure.
 * @note    It could be empty on some architectures.
 */
typedef struct {
    uint32_t  samplerate;
    uint8_t   trigger;
    uint8_t   resolution;
    uint8_t   startup;
} ADCConfig;

/**
 * @brief   Structure representing an ADC driver.
 */
struct ADCDriver {
    /**
     * @brief   Driver state.
     */
    adcstate_t                state;
    /**
     * @brief   Current configuration data.
     */
    const ADCConfig           *config;
    /**
     * @brief   Current samples buffer pointer or @p NULL.
     */
    adcsample_t               *samples;
    /**
     * @brief   Current samples buffer depth or @p 0.
     */
    size_t                    depth;
    /**
     * @brief   Current conversion group pointer or @p NULL.
     */
    const ADCConversionGroup  *grpp;
#if ADC_USE_WAIT || defined(__DOXYGEN__)
    /**
     * @brief   Waiting thread.
     */
    Thread                    *thread;
#endif
#if ADC_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
#if CH_USE_MUTEXES || defined(__DOXYGEN__)
    /**
     * @brief   Mutex protecting the peripheral.
     */
    Mutex                     mutex;
#elif CH_USE_SEMAPHORES
    Semaphore                 semaphore;
#endif
#endif /* ADC_USE_MUTUAL_EXCLUSION */
#if defined(ADC_DRIVER_EXT_FIELDS)
    ADC_DRIVER_EXT_FIELDS
#endif
    /* End of the mandatory fields.*/
    uint32_t mr;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/


#define  ADC_STARTUP_TIME_0 ADC_MR_STARTUP_SUT0
#define  ADC_STARTUP_TIME_1 ADC_MR_STARTUP_SUT8
#define  ADC_STARTUP_TIME_2 ADC_MR_STARTUP_SUT16
#define  ADC_STARTUP_TIME_3 ADC_MR_STARTUP_SUT24
#define  ADC_STARTUP_TIME_4 ADC_MR_STARTUP_SUT64
#define  ADC_STARTUP_TIME_5 ADC_MR_STARTUP_SUT80
#define  ADC_STARTUP_TIME_6 ADC_MR_STARTUP_SUT96
#define  ADC_STARTUP_TIME_7 ADC_MR_STARTUP_SUT112
#define  ADC_STARTUP_TIME_8 ADC_MR_STARTUP_SUT512
#define  ADC_STARTUP_TIME_9 ADC_MR_STARTUP_SUT576
#define  ADC_STARTUP_TIME_10 ADC_MR_STARTUP_SUT640
#define  ADC_STARTUP_TIME_11 ADC_MR_STARTUP_SUT704
#define  ADC_STARTUP_TIME_12 ADC_MR_STARTUP_SUT768
#define  ADC_STARTUP_TIME_13 ADC_MR_STARTUP_SUT832
#define  ADC_STARTUP_TIME_14 ADC_MR_STARTUP_SUT896
#define  ADC_STARTUP_TIME_15 ADC_MR_STARTUP_SUT960

#define ADC_LOW_RES ADC_MR_LOWRES_BITS_10
#define ADC_HI_RES ADC_MR_LOWRES_BITS_12

#define ADC_SLEEPMODE 1
#define ADC_NO_SLEEPMODE 0

#define ADC_CONFIG(cfg, _samplerate,_trigger,_resolution,_sleepmode,_startup) \
    (cfg)->samplerate = _samplerate; \
    (cfg)->trigger = _trigger; \
    (cfg)->resolution = _resolution; \
    (cfg)->sleepmode = _sleepmode; \
    (cfg)->startup = _startup

#define ADC_CONVERSION_CONFIG(cfg, _circular, callback, n, ...) \
    (cfg)->circular = _circular; \
    (cfg)->end_cb = callback;\
    adc_prepare_conversion(cfg,n,__VA_ARGS__)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if SAM3X_ADC_USE_ADC1 && !defined(__DOXYGEN__)
extern ADCDriver ADCD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
void adc_lld_init(void);
void adc_lld_start(ADCDriver *adcp);
void adc_lld_stop(ADCDriver *adcp);
void adc_lld_start_conversion(ADCDriver *adcp);
void adc_lld_stop_conversion(ADCDriver *adcp);
void adc_prepare_config(ADCConfig *cfg, uint32_t samplerate);
void adc_prepare_conversion(ADCConversionGroup *grpp, int n, ...);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_ADC */

#endif /* _ADC_LLD_H_ */

/** @} */

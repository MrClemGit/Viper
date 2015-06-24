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
 * @file    templates/pal_lld.c
 * @brief   PAL subsystem low level driver template.
 *
 * @addtogroup PAL
 * @{
 */

#include "hal.h"
#include "ch.h"

#if HAL_USE_PAL || defined(__DOXYGEN__)



/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* GPIO Support */
#define PIO_TYPE_Pos                  27
/* PIO Type Mask */
#define PIO_TYPE_Msk                  (0xFu << PIO_TYPE_Pos)
/* The pin is not a function pin. */
#define PIO_TYPE_NOT_A_PIN            (0x0u << PIO_TYPE_Pos)
/* The pin is controlled by the peripheral A. */
#define PIO_TYPE_PIO_PERIPH_A         (0x1u << PIO_TYPE_Pos)
/* The pin is controlled by the peripheral B. */
#define PIO_TYPE_PIO_PERIPH_B         (0x2u << PIO_TYPE_Pos)
/* The pin is controlled by the peripheral C. */
#define PIO_TYPE_PIO_PERIPH_C         (0x3u << PIO_TYPE_Pos)
/* The pin is controlled by the peripheral D. */
#define PIO_TYPE_PIO_PERIPH_D         (0x4u << PIO_TYPE_Pos)
/* The pin is an input. */
#define PIO_TYPE_PIO_INPUT            (0x5u << PIO_TYPE_Pos)
/* The pin is an output and has a default level of 0. */
#define PIO_TYPE_PIO_OUTPUT_0         (0x6u << PIO_TYPE_Pos)
/* The pin is an output and has a default level of 1. */
#define PIO_TYPE_PIO_OUTPUT_1         (0x7u << PIO_TYPE_Pos)

typedef enum _pio_type {
    PIO_NOT_A_PIN   = PIO_TYPE_NOT_A_PIN,
    PIO_PERIPH_A    = PIO_TYPE_PIO_PERIPH_A,
    PIO_PERIPH_B    = PIO_TYPE_PIO_PERIPH_B,
#if (SAM3S || SAM3N || SAM4S || SAM4E || SAM4N || SAM4C || SAM4CP || SAM4CM)
    PIO_PERIPH_C    = PIO_TYPE_PIO_PERIPH_C,
    PIO_PERIPH_D    = PIO_TYPE_PIO_PERIPH_D,
#endif
    PIO_INPUT       = PIO_TYPE_PIO_INPUT,
    PIO_OUTPUT_0    = PIO_TYPE_PIO_OUTPUT_0,
    PIO_OUTPUT_1    = PIO_TYPE_PIO_OUTPUT_1
} pio_type_t;

/*  Default pin configuration (no attribute). */
#define PIO_DEFAULT             (0u << 0)
/*  The internal pin pull-up is active. */
#define PIO_PULLUP              (1u << 0)
/*  The internal glitch filter is active. */
#define PIO_DEGLITCH            (1u << 1)
/*  The pin is open-drain. */
#define PIO_OPENDRAIN           (1u << 2)

/*  The internal debouncing filter is active. */
#define PIO_DEBOUNCE            (1u << 3)

/*  Enable additional interrupt modes. */
#define PIO_IT_AIME             (1u << 4)

/*  Interrupt High Level/Rising Edge detection is active. */
#define PIO_IT_RE_OR_HL         (1u << 5)
/*  Interrupt Edge detection is active. */
#define PIO_IT_EDGE             (1u << 6)

/*  Low level interrupt is active */
#define PIO_IT_LOW_LEVEL        (0               | 0 | PIO_IT_AIME)
/*  High level interrupt is active */
#define PIO_IT_HIGH_LEVEL       (PIO_IT_RE_OR_HL | 0 | PIO_IT_AIME)
/*  Falling edge interrupt is active */
#define PIO_IT_FALL_EDGE        (0               | PIO_IT_EDGE | PIO_IT_AIME)
/*  Rising edge interrupt is active */
#define PIO_IT_RISE_EDGE        (PIO_IT_RE_OR_HL | PIO_IT_EDGE | PIO_IT_AIME)

/*
 *  The #attribute# field is a bitmask that can either be set to PIO_DEFAULT,
 *  or combine (using bitwise OR '|') any number of the following constants:
 *     - PIO_PULLUP
 *     - PIO_DEGLITCH
 *     - PIO_DEBOUNCE
 *     - PIO_OPENDRAIN
 *     - PIO_IT_LOW_LEVEL
 *     - PIO_IT_HIGH_LEVEL
 *     - PIO_IT_FALL_EDGE
 *     - PIO_IT_RISE_EDGE
 */

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/




/**
 * \brief Configure one or more pin(s) or a PIO controller as inputs.
 * Optionally, the corresponding internal pull-up(s) and glitch filter(s) can
 * be enabled.
 *
 * \param p_pio Pointer to a PIO instance.
 * \param ul_mask Bitmask indicating which pin(s) to configure as input(s).
 * \param ul_attribute PIO attribute(s).
 */
void pio_set_input(ioportid_t pio, const ioportmask_t ul_mask,
                   const uint32_t ul_attribute) {

    PIO_DISABLE_INTERRUPT(pio, ul_mask);
    PIO_SET_PULLUP(pio, ul_mask, ul_attribute & PIO_PULLUP);
    PIO_DISABLE_GLITCH(pio, ul_mask);

    /* Configure pin as input */
    PIO_DISABLE_OUTPUT(pio, ul_mask);
    PIO_ENABLE(pio, ul_mask);
}

/**
 * \brief Configure one or more pin(s) of a PIO controller as outputs, with
 * the given default value. Optionally, the multi-drive feature can be enabled
 * on the pin(s).
 *
 * \param p_pio Pointer to a PIO instance.
 * \param ul_mask Bitmask indicating which pin(s) to configure.
 * \param ul_default_level Default level on the pin(s).
 * \param ul_multidrive_enable Indicates if the pin(s) shall be configured as
 * open-drain.
 * \param ul_pull_up_enable Indicates if the pin shall have its pull-up
 * activated.
 */
void pio_set_output(ioportid_t pio, const ioportmask_t ul_mask,
                    const uint32_t ul_attribute) {

    PIO_DISABLE_INTERRUPT(pio, ul_mask);

    PIO_SET_PULLUP(pio, ul_mask, ul_attribute & PIO_PULLUP);

    PIO_SET_OPENDRAIN(pio, ul_mask, ul_attribute & PIO_OPENDRAIN);

    //PIO_UNSET(pio, ul_mask);


    /* Configure pin(s) as output(s) */
    PIO_ENABLE_OUTPUT(pio, ul_mask);
    PIO_ENABLE(pio, ul_mask);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   STM32 I/O ports configuration.
 * @details Ports A-D(E, F, G, H) clocks enabled.
 *
 * @param[in] config    the STM32 ports configuration
 *
 * @notapi
 */
void _pal_lld_init(const PALConfig *config) {

    (void)config;
    pmc_enable_periph_clk(ID_PIOA);
    pmc_enable_periph_clk(ID_PIOB);
    pmc_enable_periph_clk(ID_PIOC);
    pmc_enable_periph_clk(ID_PIOD);
}

/**
 * @brief   Pads mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 *
 * @param[in] port      the port identifier
 * @param[in] mask      the group mask
 * @param[in] mode      the mode
 *
 * @notapi
 */
void _pal_lld_setgroupmode(ioportid_t port,
                           ioportmask_t mask,
                           iomode_t mode) {

    (void)port;
    (void)mask;
    (void)mode;

}


void pio_set_peripheral(ioportid_t port, uint32_t mask, uint32_t ph, uint32_t pullup) {
    uint32_t ul_sr;
    PIO_DISABLE_INTERRUPT(port, mask);

    switch (ph) {
        case PIO_PERIPH_A:
            ul_sr = port->PIO_ABSR;
            port->PIO_ABSR &= (~mask & ul_sr);
            break;

        case PIO_PERIPH_B:
            ul_sr = port->PIO_ABSR;
            port->PIO_ABSR = (mask | ul_sr);
            break;
    }
    /* Remove the pins from under the control of PIO */
    PIO_DISABLE(port, mask);
    PIO_SET_PULLUP(port, mask, pullup);
}

void _pal_lld_setpadmode(ioportid_t port, int pad, int mode) {

    if ((mode & 0xf0) == PAD_TO_A) {
        pio_set_peripheral(port, PIO_PAD(pad), PIO_PERIPH_A, mode & (PAL_MODE_OUTPUT_PUSHPULL|PAL_MODE_INPUT_PULLUP));
    } else if ((mode & 0xf0 ) == PAD_TO_B) {
        pio_set_peripheral(port, PIO_PAD(pad), PIO_PERIPH_B, mode & (PAL_MODE_OUTPUT_PUSHPULL|PAL_MODE_INPUT_PULLUP));
    } else {

        switch (mode & 0x0f) {
            case PAL_MODE_RESET:
            case PAL_MODE_UNCONNECTED:
                break;
            case PAL_MODE_INPUT_PULLUP:
                pio_set_input(port, PIO_PAD(pad), PIO_PULLUP);
                break;
            case PAL_MODE_INPUT:
            case PAL_MODE_INPUT_PULLDOWN:
                pio_set_input(port, PIO_PAD(pad), 0);
                break;
            case PAL_MODE_INPUT_ANALOG:
                /*TODO*/
                break;
            case PAL_MODE_OUTPUT_PUSHPULL:
                pio_set_output(port, PIO_PAD(pad), PIO_PULLUP);
                break;
            case PAL_MODE_OUTPUT_OPENDRAIN:
                pio_set_output(port, PIO_PAD(pad), PIO_OPENDRAIN);
                break;
        }
    }
}

#endif /* HAL_USE_PAL */

/** @} */

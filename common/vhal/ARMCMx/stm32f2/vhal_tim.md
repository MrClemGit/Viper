TIM notes
================================

### FREQ

STM32F401: 84MHz (max)
HSI_VALUE: 16MHz? (Internal Clock)
HSE_VALUE: 8MHz (External Clock, oscillator)
PLL_M:     8 (factor, divides HSE_VALUE)
PLL_N:	   336  (multiplier factor, 192 <= PLL_N <= 432, MCD(84M, 48M) least common value of core clock and USB clock)

PLL_VCO = PLL_N * (HSE_VALUE / PLL_M)
PLL_P: 	  2 (divider for PPL_VCO to MCU Clock)
SYSCLK = PLL_VCO / PLL_P = 168MHz
PLL_Q:	  7 (divider for PPL_VCO to special peripheral clock SDIO, USB, RNG)
CLOCK_48MHz = PLL_VCO / PLL_Q = 48MHz
# List of all SAM3X platform files.
PLATFORMSRC = ${CHIBIOS}/os/hal/platforms/SAM3X8E/hal_lld.c \
              ${CHIBIOS}/os/hal/platforms/SAM3X8E/sam3x_dma.c \
              ${CHIBIOS}/os/hal/platforms/SAM3X8E/pal_lld.c \
              ${CHIBIOS}/os/hal/platforms/SAM3X8E/ext_lld.c \
              ${CHIBIOS}/os/hal/platforms/SAM3X8E/spi_lld.c \
              ${CHIBIOS}/os/hal/platforms/SAM3X8E/serial_lld.c \
              ${CHIBIOS}/os/hal/platforms/SAM3X8E/adc_lld.c \
              ${CHIBIOS}/os/hal/platforms/SAM3X8E/i2c_lld.c \
              ${CHIBIOS}/os/hal/platforms/SAM3X8E/uart_lld.c \
              ${CHIBIOS}/os/hal/platforms/SAM3X8E/pwm_lld.c 

# Required include directories
PLATFORMINC = ${CHIBIOS}/os/hal/platforms/SAM3X8E \
              ${CHIBIOS}/os/hal/platforms/SAM3X8E/ASF/sam/utils/cmsis/sam3x/include \
              ${CHIBIOS}/os/hal/platforms/SAM3X8E/ASF/sam/utils/cmsis/sam3x/source/templates
              

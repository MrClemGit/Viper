#####################
#
# ST NUCLEO F401RE
#

#### Compiler options
ifeq ($(USE_OPT),)
  USE_OPT = -O2 -ggdb -fomit-frame-pointer -falign-functions=16 -fdiagnostics-color=auto
endif

# C specific options here (added to USE_OPT).
ifeq ($(USE_COPT),)
  USE_COPT = 
endif

# C++ specific options here (added to USE_OPT).
ifeq ($(USE_CPPOPT),)
  USE_CPPOPT = -fno-rtti
endif

# Enable this if you want the linker to remove unused code and data
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC = yes
endif

# Linker extra options here.
ifeq ($(USE_LDOPT),)
  USE_LDOPT = 
endif

# Enable this if you want link time optimizations (LTO)
ifeq ($(USE_LTO),)
  USE_LTO = no
endif

# If enabled, this option allows to compile the application in THUMB mode.
ifeq ($(USE_THUMB),)
  USE_THUMB = yes
endif

# Enable this if you want to see the full log while compiling.
ifeq ($(USE_VERBOSE_COMPILE),)
  USE_VERBOSE_COMPILE = no
endif

# Enable this if you want link time optimizations (LTO)
ifeq ($(USE_FPU),)
  USE_FPU = no
endif




#### Compiler Settings

MCU  = cortex-m3

TRGT = $(CCPATH)arm-none-eabi-
CC   = $(TRGT)gcc
CPPC = $(TRGT)g++
# Enable loading with g++ only if you need C++ runtime support.
# NOTE: You can use C++ even without C++ support if you are careful. C++
#       runtime support makes code size explode.
LD   = $(TRGT)gcc
#LD   = $(TRGT)g++
CP   = $(TRGT)objcopy
AS   = $(TRGT)gcc -x assembler-with-cpp
OD   = $(TRGT)objdump
SZ   = $(TRGT)size
HEX  = $(CP) -O ihex
BIN  = $(CP) -O binary

# ARM-specific options here
AOPT =

# THUMB-specific options here
TOPT = -mthumb -DTHUMB

# Define C warning options here
CWARN = -Wall -Wextra -Wstrict-prototypes

# Define C++ warning options here
CPPWARN = -Wall -Wextra


##### 
VHAL_PLATFORM=ARMCMx/stm32f2
BOARD_CONFIG_DIR=boards/$(BOARD)/config


##### CHECK RTOS
ifeq ($(RTOS),chibi2)

# Where to find the linker script
BOARD_LINKER_SCRIPT:=$(BOARD_CONFIG_DIR)/linker_script.ld

# Where to find ChibiOS board files
BOARD_OS_BOARD:=BCM943362WCD4

# Where to find the HAL layer
BOARD_OS_HAL:=STM32F4xx

# Where to find the ChibiOS port for the MCU
BOARD_OS_PORT:=GCC/ARMCMx/STM32F2xx

# Where to find rules
BOARD_RULES:=GCC/ARMCMx

#gcc defines
BOARD_DEFS:=

# Additional Code and Inc

BOARD_SRC = boards/$(BOARD)/port.c
#$(wildcard platform/common/libs/STM32F4xx_StdPeriph_Driver/src/*_rng.c) \
#$(wildcard platform/common/libs/STM32F4xx_StdPeriph_Driver/src/*_hash*.c) \
#$(wildcard platform/common/libs/STM32F4xx_StdPeriph_Driver/src/*_cryp*.c) \
#$(wildcard platform/common/libs/STM32F4xx_StdPeriph_Driver/src/*_rcc.c)

BOARD_INC = boards/$(BOARD)/ $(BOARD_CONFIG_DIR)

BOARD_ADDITIONAL_SRC = $(RTOSDIR)/os/os/various/chprintf.c $(RTOSDIR)/os/os/various/chrtclib.c

UPLOAD_COMMAND=cp build/viper.bin /run/media/$(shell whoami)/NUCLEO


#include def file
include $(BOARD_CONFIG_DIR)/custom.mk

BOARD_DRIVERS = $(foreach r,$(BOARD_EXT_DRIVERS),DRV_$(r))


BOARD_DEFS+= $(foreach df,$(BOARD_DRIVERS),-D$(df))
BOARD_DEFS+= $(foreach df,$(BOARD_DEFINES),-D$(df))

#BOARD_DEFS+= $(foreach df,$(NATIVE_FILES),-DNUM_$(shell echo $(df) | tr '[:lower:]' '[:upper:]')=$(shell wc -l #lang/$(df).def| cut -f1 -d ' '))


else

$(error $(RTOS) not supported!)

endif

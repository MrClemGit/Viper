
##############################################################################
# BOARD & RTOS
#

RTOSDIR = common/vos/$(RTOS)

include boards/$(BOARD)/platform.mk

include $(RTOSDIR)/rtos.mk

#
# 
##############################################################################

##############################################################################
# Project, sources and paths
#

# Project Name
PROJECT = viper

# Linker script
LDSCRIPT= $(BOARD_LINKER_SCRIPT)


COMMON_SRC = commono/platform.c
COMMON_INC = common/inc
_DRIVERS = $(foreach drv,$(BOARD_EXT_DRIVERS), common/drivers/$(drv)/$(drv).mk)
include $(_DRIVERS)


VOS_SRC=$(RTOSDIR)/vosal.c
VHAL_SRC=common/vhal/vhal.c $(wildcard common/vhal/$(VHAL_PLATFORM)/*.c) $(wildcard common/vhal/$(VHAL_PLATFORM)/src/*.c)
VHAL_INC=common/vhal/$(VHAL_PLATFORM)/inc common/vhal/$(VHAL_PLATFORM)
VM_SRC=$(wildcard lang/*.c)
VM_INC=lang/

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CSRC = $(RTOS_SRC)\
       $(VOS_SRC) \
       $(VHAL_SRC) \
       $(COMMON_SRC) \
       $(BOARD_SRC) \
       $(BOARD_ADDITIONAL_SRC) \
       $(VM_SRC) \
       $(MAINFILE)

# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CPPSRC =

# C sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACSRC =

# C++ sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACPPSRC =

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCSRC =

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCPPSRC =

# List ASM source files here
ASMSRC = $(RTOS_ASM)

INCDIR = $(RTOS_INC)\
         $(VHAL_INC) \
         $(BOARD_INC) \
         $(VM_INC) \
         $(COMMON_INC)
		 



##############################################################################
# Start of default section
#

# List all default C defines here, like -D_DEBUG=1
DDEFS = $(BOARD_DEFS) -DRTOS_$(RTOS) -DVM_VERSION=\"$(VM_VERSION)\"

ifeq ($(DEBUG),1)
	DDEFS+= -DVM_DEBUG -DVM_DEBUG_LEVEL=$(DEBUG_LEVEL)
endif

ifeq ($(VM_SRC),)
	DDEFS+= -DNO_VBL
endif


# List all default ASM defines here, like -D_DEBUG=1
DADEFS =

# List all default directories to look for include files here
DINCDIR =

# List the default directory to look for the libraries here
DLIBDIR =

# List all default libraries here
DLIBS = -lm

#
# End of default section
##############################################################################

##############################################################################
# Start of user section
#

# List all user C define here, like -D_DEBUG=1
UDEFS =

# Define ASM defines here
UADEFS =

# List all user directories here
UINCDIR =

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

#
# End of user defines
##############################################################################

RULESPATH = common

include $(RULESPATH)/rules.mk

COLSTART_=\033[0;31m
COLEND=\033[0m

upload: all
	$(UPLOAD_COMMAND)

oal_conf.h:
	@printf "\n================\n"
	@printf "     VIPER\n"
	@printf "================\n"
	@printf "$(COLSTART_)Using gcc at:$(COLEND) $(CCPATH)\n"
	@printf "$(COLSTART_)Building for:$(COLEND) $(BOARD)\n"
	@printf "$(COLSTART_)SOURCE FILES$(COLEND)\n"
	@printf "$(VM_SRC)$(CSRC)\n"
	@printf "$(COLSTART_)INCLUDE PATHS$(COLEND)\n"
	@printf "$(VM_INC)$(INCDIR)\n"
	@printf "$(COLSTART_)DEFINES$(COLEND)\n"
	@printf "$(DDEFS)\n"
	@printf "\n==================================================\nCREATING $(BOARD_CONFIG_DIR)/oal_conf.h\n==================================================\n"
	@printf '/* VIPER $(BOARD_CONFIG_DIR)/oal_conf.h */\n' > $(BOARD_CONFIG_DIR)/oal_conf.h
	$(foreach drv,$(BOARD_EXT_DRIVERS),@printf '#include "$(drv)_driver.h"\n' >> $(BOARD_CONFIG_DIR)/oal_conf.h ;)
	@printf '#define EXTERNAL_DRIVERS ' >> $(BOARD_CONFIG_DIR)/oal_conf.h
	$(foreach drv,$(BOARD_EXT_DRIVERS),@printf '$(drv)_DEF ' >> $(BOARD_CONFIG_DIR)/oal_conf.h ;)
	@printf "\n" >> $(BOARD_CONFIG_DIR)/oal_conf.h


production: all
	@printf "\nGenerating VM info...\n"
	@echo "{\"version\":\"$(VM_VERSION)\",\"board\":\"$(BOARD)\",\"name\":\"$(VM_NAME)\",\"desc\":\"$(VM_DESC)\"}" > build/viper.vm
	@printf "Done!\n"
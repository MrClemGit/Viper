
CHIBIOS = $(RTOSDIR)/os
# Imported source files and paths
include $(CHIBIOS)/boards/$(BOARD_OS_BOARD)/board.mk
include $(CHIBIOS)/os/hal/platforms/$(BOARD_OS_HAL)/platform.mk
include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/ports/$(BOARD_OS_PORT)/port.mk
include $(CHIBIOS)/os/kernel/kernel.mk


RTOS_SRC = $(PORTSRC) \
       $(KERNSRC) \
       $(TESTSRC) \
       $(HALSRC) \
       $(PLATFORMSRC) \
       $(BOARDSRC)

RTOS_INC = $(PORTINC) $(KERNINC) $(TESTINC) \
         $(HALINC) $(PLATFORMINC) $(BOARDINC) \
         $(CHIBIOS)/os/various \
         $(RTOSDIR)

RTOS_ASM = $(PORTASM)       
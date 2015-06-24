ifndef BOARD
$(error BOARD not specified: use make BOARD=board_name)
endif

ifndef TESTFILE
	MAINFILE=common/main.c
else
	MAINFILE=boards/$(BOARD)/tests/$(TESTFILE) common/viper_test.c
endif

ifndef HOST_PLATFORM
	HOST_PLATFORM=linux64
endif

ifndef RTOS
	RTOS=chibi2
endif

ifndef DEBUG
	DEBUG=0
else
	DEBUG=1
ifndef DEBUG_LEVEL
	DEBUG_LEVEL=0
endif
endif

ifndef VM_VERSION
	VM_VERSION="0.1.8"
endif

ifndef VM_DESC
	VM_DESC="---"
endif


ifndef VM_NAME
	VM_NAME=$(BOARD)_$(VM_VERSION)
endif



CCPATH=$(realpath .)/../../../tools/$(HOST_PLATFORM)/gcc/arm/bin/
NATIVE_FILES=natives
#natives_str natives_bytes natives_bytearray natives_list natives_tuple natives_range natives_fset natives_set natives_dict natives_function

include common/common.mk




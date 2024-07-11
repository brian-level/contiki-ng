BOARD?=brd4210a
TARGET?=gecko2

CONTIKI_PROJECT = subghzcpu

all: $(CONTIKI_PROJECT)

CFLAGS += -DSL_BOARD_NAME=\"${BOARD}\" -D__ZEPHYR_SIM__ -DCONFIG_CONTIKI
ifeq ("${ROOT_NODE}", "root")
    CFLAGS += -DROOT_NODE
endif

ifeq ("${BOARD}", "brd4210a")
    CFLAGS += -DBOARD_IS_DK=1
endif
CFLAGS += -Wno-pointer-sign -Wno-format -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-label

MODULES += $(CONTIKI_NG_NET_DIR)/ipv6/multicast

LEVEL_C_SRCS += node_app.c
LEVEL_C_SRCS += contikiaddr.c
LEVEL_INC_PATHS += build/${BOARD}

PROJECT_SOURCEFILES += ${notdir ${LEVEL_C_SRCS}}
EXTERNALDIRS += $(LEVEL_SRC_PATHS) ${LEVEL_INC_PATHS}

#include src/Makefile

MODULES += os/services/shell

CONTIKI = ../../contiki-ng
include $(CONTIKI)/Makefile.include



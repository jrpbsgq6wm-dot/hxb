############################################################################
## Copyright (C) 2009 ~ 2017 Beijing Star Test High-Tech Co., Ltd.
## All rights reserved by www.startest.cn, Inc.
## Makefile : TODO
## Maintainer  : $Id: Makefile 19762 2022-07-05 09:01:58Z wucheng $
## Author      : WuCheng <wucheng@startest.net>
## Create Date : 2022-07-05
## arm-xilinx-linux-gnueabi-gcc beam.c -o app -lpthread -lm
############################################################################

.PHONY : all install clean

VERSION = release
VERSION ?= debug

PROJECT_NAME = CuBeamOne 
EDITION = 00.00.01

MKDIR = mkdir
RM = rm
RMFLAGS = -fr

CC = /tools/Xilinx/Vitis/2023.1/gnu/aarch32/lin/gcc-arm-linux-gnueabi/bin/arm-linux-gnueabihf-gcc

ifeq (${VERSION}, debug)
CCFLAG = -pipe -O0 -ggdb -Wall -W -w -c -g 
else
CCFLAG = -pipe -O2 -Wall -W -w -c -g
endif

LDBINFLAG = -lrt -lpthread -lm -static-libgcc

INCDIRPATH = -I. -I./include
LIBDIRPATH = -L./lib

SRC_DIR = src
EXE_DIR = $(SRC_DIR)
INC_DIR = include
OBJ_DIR = .objs
DEP_DIR = .deps
DIRS = $(OBJ_DIR) $(DEP_DIR) 

EXE_BIN = app
EXE_BIN := $(addprefix $(EXE_DIR)/, $(EXE_BIN))

INSTALL_DIR = build/program/app

SRC_LIST = $(wildcard $(SRC_DIR)/*.c)  				#把所有的.c文件赋值给SRC_LIST;
SRC_LIST := $(subst $(SRC_DIR)/, , $(SRC_LIST)) 	#可以用notdir$(SRC_DIR)去除路径；

OBJ_LIST = $(SRC_LIST:.c=.o)						#把所有的.c文件替换成.o文件
OBJ_LIST := $(addprefix $(OBJ_DIR)/, $(OBJ_LIST))	#给OBJ_LIST中所有变量加上.objs前缀

INC_LIST = $(wildcard $(INC_DIR)/*.h)
INC_LIST := $(subst $(INC_DIR)/, ,$(INC_LIST))

DEP_LIST = $(INC_LIST:.h=.dep)						#把所有SRC_LIST中的所有.c文件替换成.dep文件
DEP_LIST := $(addprefix $(DEP_DIR)/, $(DEP_LIST))	#给DEP_LIST中的所有文件加上.deps前缀

ifeq ("$(wildcard $(OBJ_DIR))", "")		
DEP_DIR_OBJS := $(OBJ_DIR)
endif
ifeq ("$(wildcard $(DEP_DIR))", "")
DEP_DIR_DEPS := $(DEP_DIR)
endif

all: ${EXE_BIN}
	@echo "=========================================================="
	@echo "        Project ${PROJECT_NAME} Build All Done."
	@echo "    [ ${VERSION} edition, version ${EDITION} ]"
	@echo "=========================================================="
	@echo ""

ifneq ($(MAKECMDGOALS), clean)
include $(DEP_LIST)
endif

$(DIRS):
	$(MKDIR) $@

${EXE_BIN} : ${OBJ_LIST}
	@${CC} -o $@ $(filter %.o, $^) ${LIBDIRPATH} ${LDBINFLAG}
	@echo "[M] $@"

${OBJ_DIR}/%.o : ${DEP_DIR_OBJS} $(SRC_DIR)/%.c
	@${CC} ${CCFLAG} ${INCDIRPATH} -o $@ -c $(filter %.c, $^)
	@echo "[M] $@"

${DEP_DIR}/%.dep : ${DEP_DIR_DEPS} $(SRC_DIR)/%.c
	@echo "Creating $@ ..."
	@set -e; \
	${RM} ${RMFLAGS} $@.tmp; \
	${CC} -E -MM ${INCDIRPATH} $(filter %.c, $^) > $@.tmp; \
	sed 's,\(.*\)\.o[ :]*,${OBJ_DIR}/\1.o $@: ,g' < $@.tmp > $@; \
	${RM} ${RMFLAGS} $@.tmp

clean:
	${RM} ${RMFLAGS} ${EXE_BIN} ${DIRS}

#${CCFLAG}

default: all

BUILD_TYPE ?= Release
BUILD_DIR := build/${BUILD_TYPE}
ROOT_DIR := $(shell pwd)

NPROCS:=1
OS:=$(shell uname -s)

ifeq ($(OS),Linux)
	NPROCS := $(shell nproc)
else ifeq ($(OS),Darwin)
	NPROCS := $(shell sysctl hw.ncpu | awk '{print $$2}')
endif

${BUILD_DIR}/Makefile: CMakeLists.txt
	@mkdir -p ${BUILD_DIR}
	@cd ${BUILD_DIR} && cmake ${ROOT_DIR} -DCMAKE_BUILD_TYPE=${BUILD_TYPE}

all: ${BUILD_DIR}/Makefile
	@${MAKE} -C ${BUILD_DIR} -j ${NPROCS}

clean:
	@${RM} -rf ${BUILD_DIR}

.PHONE: all clean

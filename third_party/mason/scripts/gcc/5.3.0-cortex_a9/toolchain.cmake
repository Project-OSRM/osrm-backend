SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR arm)
SET(CMAKE_SYSTEM_VERSION 1)

SET(CMAKE_C_COMPILER ${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabi-gcc)
SET(CMAKE_CXX_COMPILER ${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabi-g++)
SET(CMAKE_RANLIB ${MASON_XC_ROOT}/root/bin/arm-cortex_a9-linux-gnueabi-ranlib)

SET(CMAKE_C_FLAGS "-mtune=cortex-a9 -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 ${CFLAGS}")
SET(CMAKE_CXX_FLAGS "-mtune=cortex-a9 -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 ${CFLAGS}")

SET(CMAKE_FIND_ROOT_PATH ${MASON_XC_ROOT}/root)

SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

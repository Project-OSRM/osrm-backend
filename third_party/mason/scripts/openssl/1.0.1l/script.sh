#!/usr/bin/env bash

MASON_NAME=openssl
MASON_VERSION=1.0.1l
MASON_LIB_FILE=lib/libssl.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/openssl.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        ftp://ftp.openssl.org/source/old/1.0.1/openssl-1.0.1l.tar.gz \
        448aaf41b40d9ff0a1722a2838fb48f78b95dfa4

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/openssl-${MASON_VERSION}
}

function mason_prepare_compile {
    MASON_MAKEDEPEND="gccmakedep"

    if [ ${MASON_PLATFORM} = 'osx' ]; then
        MASON_MAKEDEPEND="makedepend"
        MASON_OS_COMPILER="darwin64-x86_64-cc enable-ec_nistp_64_gcc_128"
    elif [ ${MASON_PLATFORM} = 'linux' ]; then
        MASON_OS_COMPILER="linux-x86_64 enable-ec_nistp_64_gcc_128"
    elif [[ ${MASON_PLATFORM} == 'android' ]]; then
        COMMON="-fpic -ffunction-sections -funwind-tables -fstack-protector-strong -no-canonical-prefixes -fno-integrated-as -O2 -g -DNDEBUG -fomit-frame-pointer -fstrict-aliasing -Wno-invalid-command-line-argument -Wno-unused-command-line-argument -no-canonical-prefixes"
        if [ ${MASON_ANDROID_ABI} = 'arm-v5' ]; then
            MASON_OS_COMPILER="linux-armv4 -march=armv5te -mtune=xscale -msoft-float -fuse-ld=gold $COMMON"
        elif [ ${MASON_ANDROID_ABI} = 'arm-v7' ]; then
            MASON_OS_COMPILER="linux-armv4 -march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp -Wl,--fix-cortex-a8 -fuse-ld=gold $COMMON"
        elif [ ${MASON_ANDROID_ABI} = 'x86' ]; then
            MASON_OS_COMPILER="linux-elf -march=i686 -msse3 -mfpmath=sse -fuse-ld=gold $COMMON"
        elif [ ${MASON_ANDROID_ABI} = 'mips' ]; then
            MASON_OS_COMPILER="linux-generic32 $COMMON"
        elif [ ${MASON_ANDROID_ABI} = 'arm-v8' ]; then
            MASON_OS_COMPILER="linux-generic64 enable-ec_nistp_64_gcc_128 -fuse-ld=gold $COMMON"
        elif [ ${MASON_ANDROID_ABI} = 'x86-64' ]; then
            MASON_OS_COMPILER="linux-x86_64 enable-ec_nistp_64_gcc_128 -march=x86-64 -msse4.2 -mpopcnt -m64 -mtune=intel -fuse-ld=gold $COMMON"
        elif [ ${MASON_ANDROID_ABI} = 'mips-64' ]; then
            MASON_OS_COMPILER="linux-generic32 $COMMON"
        fi
    fi
}

function mason_compile {
    ./Configure \
        --prefix=${MASON_PREFIX} \
        enable-tlsext \
        -no-dso \
        -no-hw \
        -no-comp \
        -no-idea \
        -no-mdc2 \
        -no-rc5 \
        -no-zlib \
        -no-shared \
        -no-ssl2 \
        -no-ssl3 \
        -no-krb5 \
        -fPIC \
        -DOPENSSL_PIC \
        -DOPENSSL_NO_DEPRECATED \
        -DOPENSSL_NO_COMP \
        -DOPENSSL_NO_HEARTBEATS \
        --openssldir=${MASON_PREFIX}/etc/openssl \
        ${MASON_OS_COMPILER}

    make depend MAKEDEPPROG=${MASON_MAKEDEPEND}

    make

    # https://github.com/openssl/openssl/issues/57
    make install_sw
}

function mason_clean {
    make clean
}

mason_run "$@"

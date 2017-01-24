#!/usr/bin/env bash

MASON_NAME=libcrypto
MASON_VERSION=1.0.1p
MASON_LIB_FILE=lib/libcrypto.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/openssl.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        ftp://ftp.openssl.org/source/old/1.0.1/openssl-${MASON_VERSION}.tar.gz \
        db77eba6cc1f9e50f61a864c07d09ecd0154c84d

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
        COMMON="-fPIC -ffunction-sections -funwind-tables -fstack-protector-strong -no-canonical-prefixes -fno-integrated-as -O2 -g -DNDEBUG -fomit-frame-pointer -fstrict-aliasing -Wno-invalid-command-line-argument -Wno-unused-command-line-argument -no-canonical-prefixes"
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
    NO_ASM=

    # Work around a Android 6.0 TEXTREL exception. See https://github.com/mapbox/mapbox-gl-native/issues/2772
    if [[ ${MASON_PLATFORM} == 'android' ]]; then
        if [ ${MASON_ANDROID_ABI} = 'x86' ]; then
            NO_ASM=-no-asm
        fi
    fi

    ./Configure \
        --prefix=${MASON_PREFIX} \
        enable-tlsext \
        ${NO_ASM} \
        -no-dso \
        -no-hw \
        -no-engines \
        -no-comp \
        -no-gmp \
        -no-zlib \
        -no-shared \
        -no-ssl2 \
        -no-ssl3 \
        -no-krb5 \
        -no-camellia \
        -no-capieng \
        -no-cast \
        -no-dtls \
        -no-gost \
        -no-idea \
        -no-jpake \
        -no-md2 \
        -no-mdc2 \
        -no-rc5 \
        -no-rdrand \
        -no-ripemd \
        -no-rsax \
        -no-sctp \
        -no-seed \
        -no-sha0 \
        -no-whirlpool \
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

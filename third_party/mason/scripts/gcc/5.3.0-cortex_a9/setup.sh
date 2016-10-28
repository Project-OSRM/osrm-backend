#!/usr/bin/env bash

if [ `uname -s` = 'Darwin' ]; then
    MASON_HOST_PLATFORM="osx"
elif [ `uname -s` = 'Linux' ]; then
    MASON_HOST_PLATFORM="linux"
fi

URL_MASON_BINARIES="https://mason-binaries.s3.amazonaws.com/prebuilt"

if [ ! -f "${MASON_PREFIX}/${MASON_LIB_FILE}" ] ; then
    mkdir -p ${MASON_ROOT}/.cache
    if [ $MASON_HOST_PLATFORM = "osx" ]; then
        URL="${URL_MASON_BINARIES}/osx-x86_64/gcc-5.3.0-arm-v7.dmg"
        FILE="${MASON_ROOT}/.cache/osx-x86_64-gcc-5.3.0-arm-v7.dmg"
    elif [ $MASON_HOST_PLATFORM = "linux" ]; then
        URL="${URL_MASON_BINARIES}/linux-$(uname -m)/gcc-5.3.0-arm-v7.tar.bz2"
        FILE="${MASON_ROOT}/.cache/linux-$(uname -m)-gcc-5.3.0-arm-v7.tar.bz2"
    fi
    if [ ! -f ${FILE} ] ; then
        mason_step "Downloading ${URL}..."
        curl --retry 3 ${MASON_CURL_ARGS} -f -L ${URL} -o ${FILE}.tmp && \
            mv ${FILE}.tmp ${FILE}
    fi
    mkdir -p ${MASON_PREFIX}/root
    if [ $MASON_HOST_PLATFORM = "osx" ]; then
        hdiutil attach -quiet -readonly -mountpoint ${MASON_PREFIX}/root ${FILE}
    elif [ $MASON_HOST_PLATFORM = "linux" ]; then
        tar xf "${FILE}" --directory "${MASON_PREFIX}/root" --strip-components=1
    fi
fi

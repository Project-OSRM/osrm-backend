#!/usr/bin/env bash

MASON_NAME=webp
MASON_VERSION=0.6.0
MASON_LIB_FILE=lib/libwebp.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/libwebp.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://downloads.webmproject.org/releases/webp/libwebp-$MASON_VERSION.tar.gz \
        7669dc9a7c110cafc9e352d3abc1753bcb465dc0

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/libwebp-${MASON_VERSION}
}

function mason_compile {

    # note CFLAGS overrides defaults
    # -fvisibility=hidden -Wall -Wdeclaration-after-statement -Wextra -Wfloat-conversion -Wformat -Wformat-nonliteral -Wformat -Wformat-security -Wmissing-declarations -Wmissing-prototypes -Wold-style-definition -Wshadow -Wshorten-64-to-32 -Wunreachable-code -Wunused -Wvla -g -O2
    # so we need to add optimization flags back
    export CFLAGS="${CFLAGS:-} -O3 -DNDEBUG -fvisibility=hidden -Wall"

    # -mfpu=neon is not enabled by default for cortex_a9 because
    # it affects floating point precision. webp is fine with it.
    # See: https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html
    # this avoids the error from webp of:
    # #error You must enable NEON instructions (e.g. -mfloat-abi=softfp -mfpu=neon) to use arm_neon.h
    if [[ ${MASON_PLATFORM_VERSION} == "cortex_a9" ]]; then
        CFLAGS="${CFLAGS:-} -mfloat-abi=softfp -mfpu=neon"
    fi

    # TODO(springmeyer) - why is --enable-swap-16bit-csp enabled?
    ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --enable-static \
        --disable-shared \
        --with-pic \
        --disable-cwebp \
        --disable-dwebp \
        --enable-swap-16bit-csp \
        --disable-gl \
        --disable-png \
        --disable-jpeg \
        --disable-tiff \
        --disable-gif \
        --disable-wic \
        --disable-dependency-tracking

    make V=1 -j${MASON_CONCURRENCY}
    make install -j${MASON_CONCURRENCY}
}

function mason_strip_ldflags {
    ldflags=()
    while [[ $1 ]]
    do
        case "$1" in
            -lwebp)
                shift
                ;;
            -L*)
                shift
                ;;
            *)
                ldflags+=("$1")
                shift
                ;;
        esac
    done
    echo "${ldflags[@]}"
}

function mason_ldflags {
    mason_strip_ldflags $(`mason_pkgconfig` --static --libs)
}

function mason_clean {
    make clean
}

mason_run "$@"

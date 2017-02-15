#!/usr/bin/env bash

MASON_NAME=webp
MASON_VERSION=0.5.1
MASON_LIB_FILE=lib/libwebp.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/libwebp.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://downloads.webmproject.org/releases/webp/libwebp-$MASON_VERSION.tar.gz \
        7c2350c6524e8419e6b541a9087607c91c957377

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/libwebp-${MASON_VERSION}
}

function mason_compile {
    export CFLAGS="${CFLAGS:-} -Os"

    # -mfpu=neon is not enabled by default for cortex_a9 because
    # it affects floating point precision. webp is fine with it.
    # See: https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html
    if [[ ${MASON_PLATFORM_VERSION} == "cortex_a9" ]]; then
        CFLAGS="${CFLAGS:-} -mfloat-abi=softfp -mfpu=neon"
    fi

    ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --enable-static \
        --disable-shared \
        --with-pic \
        --enable-libwebpdecoder \
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

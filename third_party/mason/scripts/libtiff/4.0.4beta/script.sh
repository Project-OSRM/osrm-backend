#!/usr/bin/env bash

MASON_NAME=libtiff
MASON_VERSION=4.0.4beta
MASON_LIB_FILE=lib/libtiff.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/libtiff-4.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://download.osgeo.org/libtiff/tiff-${MASON_VERSION}.tar.gz \
        7bbd91b09cef1a4c29d3cccb7e656ee32587e5ef

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/tiff-${MASON_VERSION}
}

function mason_prepare_compile {
    cd $(dirname ${MASON_ROOT})
    ${MASON_DIR}/mason install jpeg_turbo 1.4.0
    MASON_JPEG=$(${MASON_DIR}/mason prefix jpeg_turbo 1.4.0)
    SYSTEM_ZLIB="/usr"
}


function mason_compile {
    ./configure --prefix=${MASON_PREFIX} \
    ${MASON_HOST_ARG} \
    --enable-static --disable-shared \
    --disable-dependency-tracking \
    --disable-cxx \
    --enable-defer-strile-load \
    --enable-chunky-strip-read \
    --with-jpeg-include-dir=${MASON_JPEG}/include \
    --with-jpeg-lib-dir=${MASON_JPEG}/lib \
    --with-zlib-include-dir=${SYSTEM_ZLIB}/include \
    --with-zlib-lib-dir=${SYSTEM_ZLIB}/lib \
    --disable-lzma --disable-jbig --disable-mdi \
    --without-x

    make -j${MASON_CONCURRENCY} V=1
    make install
}

function mason_ldflags {
    echo "-ltiff -ljpeg -lz"
}

function mason_clean {
    make clean
}

mason_run "$@"

#!/usr/bin/env bash

MASON_NAME=libxml2
MASON_VERSION=2.9.3
MASON_LIB_FILE=lib/libxml2.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/libxml-2.0.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        ftp://xmlsoft.org/libxml2/libxml2-${MASON_VERSION}.tar.gz \
        5b5ff1b6cc9b5aa292a3748a45fc22fa9b46dc8e

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    # note --with-writer for osmium
    ./configure --prefix=${MASON_PREFIX} \
        --enable-static --disable-shared ${MASON_HOST_ARG} \
        --with-writer \
        --with-xptr \
        --with-xpath \
        --with-xinclude \
        --with-threads \
        --with-tree \
        --with-catalog \
        --without-icu \
        --without-zlib \
        --without-python \
        --without-legacy \
        --without-iconv \
        --without-debug \
        --without-docbook \
        --without-ftp \
        --without-html \
        --without-http \
        --without-sax1 \
        --without-schemas \
        --without-schematron \
        --without-valid \
        --without-modules \
        --without-lzma \
        --without-readline \
        --without-regexps \
        --without-c14n
    make install -j${MASON_CONCURRENCY}
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include/libxml2"
}

function mason_ldflags {
    echo "-L${MASON_PREFIX}/lib -lxml2 -lpthread -lm"
}

function mason_clean {
    make clean
}

mason_run "$@"

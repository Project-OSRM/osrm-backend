#!/usr/bin/env bash

MASON_NAME=proj
MASON_VERSION=4.8.0
MASON_LIB_FILE=lib/libproj.a
#MASON_PKGCONFIG_FILE=lib/pkgconfig/proj.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://download.osgeo.org/proj/proj-${MASON_VERSION}.tar.gz \
        531953338fd3167670cafb44ca59bd58eaa8712d

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    curl --retry 3 -f -# -L http://download.osgeo.org/proj/proj-datumgrid-1.5.zip -o proj-datumgrid-1.5.zip
    cd nad
    unzip -o ../proj-datumgrid-1.5.zip
    cd ../
    ./configure --prefix=${MASON_PREFIX} \
    --without-mutex ${MASON_HOST_ARG} \
    --with-jni=no \
    --enable-static \
    --disable-shared \
    --disable-dependency-tracking

    make -j${MASON_CONCURRENCY}
    make install
}

function mason_ldflags {
    echo "-lproj"
}

function mason_clean {
    make clean
}

mason_run "$@"

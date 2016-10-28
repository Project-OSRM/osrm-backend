#!/usr/bin/env bash

MASON_NAME=icu
MASON_VERSION=55.1
MASON_LIB_FILE=lib/libicuuc.a
#MASON_PKGCONFIG_FILE=lib/pkgconfig/icu-uc.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://download.icu-project.org/files/icu4c/55.1/icu4c-55_1-src.tgz \
        0b38bcdde97971917f0039eeeb5d070ed29e5ad7

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}
}

function mason_compile {
    # note: -DUCONFIG_NO_BREAK_ITERATION=1 is desired by mapnik (for toTitle)
    # http://www.icu-project.org/apiref/icu4c/uconfig_8h_source.html
    export ICU_CORE_CPP_FLAGS="-DU_CHARSET_IS_UTF8=1"
    # disabled due to breakage with node-mapnik on OS X: https://github.com/mapnik/mapnik-packaging/issues/98
    # -DU_USING_ICU_NAMESPACE=0 -DU_STATIC_IMPLEMENTATION=1 -DU_TIMEZONE=0 -DUCONFIG_NO_LEGACY_CONVERSION=1 -DUCONFIG_NO_FORMATTING=1 -DUCONFIG_NO_TRANSLITERATION=1 -DUCONFIG_NO_REGULAR_EXPRESSIONS=1"
    export ICU_EXTRA_CPP_FLAGS="${ICU_CORE_CPP_FLAGS} -DUCONFIG_NO_COLLATION=1"
    cd ./source
    CFLAGS="${CFLAGS} -fvisibility=hidden"
    CXXFLAGS="${CXXFLAGS} -fvisibility=hidden"
    ./configure ${MASON_HOST_ARG} --prefix=${MASON_PREFIX} \
    --with-data-packaging=archive \
    --enable-renaming \
    --enable-strict \
    --enable-release \
    --enable-static \
    --enable-draft \
    --enable-tools \
    --disable-rpath \
    --disable-debug \
    --disable-shared \
    --disable-tests \
    --disable-extras \
    --disable-tracing \
    --disable-layout \
    --disable-icuio \
    --disable-samples \
    --disable-dyload

    make -j${MASON_CONCURRENCY}
    make install
}

function mason_ldflags {
    echo "-licuuc"
}

function mason_clean {
    make clean
}

mason_run "$@"

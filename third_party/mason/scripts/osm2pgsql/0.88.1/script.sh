#!/usr/bin/env bash

MASON_NAME=osm2pgsql
MASON_VERSION=0.88.1
MASON_LIB_FILE=bin/osm2pgsql

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/openstreetmap/osm2pgsql/archive/0.88.1.tar.gz \
        24d58477f3f9b8406af115f9eb2bab51ac64e09d

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_prepare_compile {
    cd $(dirname ${MASON_ROOT})
    # set up to fix libtool .la files
    # https://github.com/mapbox/mason/issues/61
    if [[ $(uname -s) == 'Darwin' ]]; then
        FIND="\/Users\/travis\/build\/mapbox\/mason"
    else
        FIND="\/home\/travis\/build\/mapbox\/mason"
    fi
    REPLACE="$(pwd)"
    REPLACE=${REPLACE////\\/}
    ${MASON_DIR}/mason install boost 1.57.0
    MASON_BOOST=$(${MASON_DIR}/mason prefix boost 1.57.0)
    MASON_BOOST_LIBS=$(pwd)/mason_packages/.link/lib
    ${MASON_DIR}/mason install boost_libsystem 1.57.0
    ${MASON_DIR}/mason link boost_libsystem 1.57.0
    ${MASON_DIR}/mason install boost_libthread 1.57.0
    ${MASON_DIR}/mason link boost_libthread 1.57.0
    MASON_BOOST_SYSTEM=$(${MASON_DIR}/mason prefix boost_libsystem 1.57.0)
    ${MASON_DIR}/mason install boost_libfilesystem 1.57.0
    ${MASON_DIR}/mason link boost_libfilesystem 1.57.0
    ${MASON_DIR}/mason install libxml2 2.9.2
    ${MASON_DIR}/mason link libxml2 2.9.2
    MASON_XML2=$(${MASON_DIR}/mason prefix libxml2 2.9.2)
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_XML2}/bin/xml2-config
    ${MASON_DIR}/mason install geos 3.4.2
    ${MASON_DIR}/mason link geos 3.4.2
    MASON_LINKED_HEADERS=$(pwd)/mason_packages/.link/include
    MASON_GEOS=$(${MASON_DIR}/mason prefix geos 3.4.2)
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_GEOS}/bin/geos-config
    ${MASON_DIR}/mason install proj 4.8.0
    MASON_PROJ=$(${MASON_DIR}/mason prefix proj 4.8.0)
    ${MASON_DIR}/mason install bzip 1.0.6
    MASON_BZIP=$(${MASON_DIR}/mason prefix bzip 1.0.6)
    # depends on sudo apt-get install zlib1g-dev
    ${MASON_DIR}/mason install zlib system
    MASON_ZLIB=$(${MASON_DIR}/mason prefix zlib system)
    ${MASON_DIR}/mason install protobuf 2.6.1
    MASON_PROTOBUF=$(${MASON_DIR}/mason prefix protobuf 2.6.1)
    ${MASON_DIR}/mason install protobuf_c 1.1.0
    MASON_PROTOBUF_C=$(${MASON_DIR}/mason prefix protobuf_c 1.1.0)
    ${MASON_DIR}/mason install libpq 9.4.1
    MASON_LIBPQ=$(${MASON_DIR}/mason prefix libpq 9.4.1)
}

function mason_compile {
    mason_step "Loading patch 'https://github.com/mapbox/mason/blob/${MASON_SLUG}/patch.diff'..."
    curl --retry 3 -s -f -# -L \
      https://raw.githubusercontent.com/mapbox/mason/${MASON_SLUG}/patch.diff \
      -O || (mason_error "Could not find patch for ${MASON_SLUG}" && exit 1)
    patch -N -p0 < ./patch.diff
    NOCONFIGURE=1 ./autogen.sh
    # --without-lockfree to workaround https://github.com/openstreetmap/osm2pgsql/issues/196
    # parse-o5m.cpp:405:58: error: invalid suffix on literal
    LDFLAGS="-L${MASON_BOOST_LIBS} -lboost_thread ${LDFLAGS}"
    if [[ $(uname -s) == 'Linux' ]]; then
        LDFLAGS="${LDFLAGS} -lrt"
    fi
    # -I${MASON_GEOS_HEADER} works around geos-config hardcoding the platform - which breaks if built on OS X 10.10 but building on 10.11
    CFLAGS="-I${MASON_LINKED_HEADERS} -I${MASON_LINKED_HEADERS}/libxml2 ${CFLAGS}" CXXFLAGS="-I${MASON_LINKED_HEADERS} -I${MASON_LINKED_HEADERS}/libxml2 -Wno-reserved-user-defined-literal ${CXXFLAGS} -I${MASON_PROTOBUF_C}/include" LDFLAGS="${LDFLAGS}" ./configure \
        --enable-static --disable-shared \
        ${MASON_HOST_ARG} \
        --prefix=${MASON_PREFIX} \
        --without-lockfree \
        --with-boost=${MASON_BOOST} \
        --with-boost-libdir=${MASON_BOOST_LIBS} \
        --with-libxml2=${MASON_XML2}/bin/xml2-config \
        --with-zlib=${MASON_ZLIB} \
        --with-bzip2=${MASON_BZIP} \
        --with-geos=${MASON_GEOS}/bin/geos-config \
        --with-proj=${MASON_PROJ} \
        --with-protobuf-c=${MASON_PROTOBUF_C} \
        --with-protobuf-c-inc=${MASON_PROTOBUF_C}/include \
        --with-protobuf-c-lib=${MASON_PROTOBUF_C}/lib \
        --with-postgresql=${MASON_LIBPQ}/bin/pg_config

    make -j${MASON_CONCURRENCY}
    make install
}

function mason_cflags {
    :
}

function mason_ldflags {
    :
}

function mason_clean {
    make clean
}

mason_run "$@"

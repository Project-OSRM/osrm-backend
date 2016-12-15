#!/usr/bin/env bash

MASON_NAME=osrm
MASON_VERSION=0.4.1
MASON_LIB_FILE=lib/libOSRM.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/Project-OSRM/osrm-backend/archive/v${MASON_VERSION}.tar.gz \
        71e553834fa571854294626d4f4d5000ae530ed6

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/osrm-backend-${MASON_VERSION}
}

function dep() {
    ${MASON_DIR}/mason install $1 $2
    ${MASON_DIR}/mason link $1 $2
}

function all_deps() {
    dep cmake 3.2.2 &
    dep lua 5.3.0 &
    dep luabind dev &
    dep boost 1.57.0 &
    dep boost_libsystem 1.57.0 &
    dep boost_libthread 1.57.0 &
    dep boost_libfilesystem 1.57.0 &
    dep boost_libprogram_options 1.57.0 &
    dep boost_libregex 1.57.0 &
    dep boost_libiostreams 1.57.0 &
    dep boost_libtest 1.57.0 &
    dep boost_libdate_time 1.57.0 &
    dep expat 2.1.0 &
    dep stxxl 1.4.1 &
    dep osmpbf 1.3.3 &
    dep protobuf 2.6.1 &
    dep bzip 1.0.6 &
    dep zlib system &
    dep tbb 43_20150316 &
    wait
}

function mason_prepare_compile {
    cd $(dirname ${MASON_ROOT})
    all_deps
    MASON_HOME=$(pwd)/mason_packages/.link
    PATH=${MASON_HOME}/bin:$PATH
    PKG_CONFIG_PATH=${MASON_HOME}/lib/pkgconfig
    LINK_FLAGS=""
    if [[ $(uname -s) == 'Linux' ]]; then
        LINK_FLAGS="${LINK_FLAGS} "'-Wl,-z,origin -Wl,-rpath=\$ORIGIN'
    fi
}

function mason_compile {
    mkdir build
    cd build
    cmake
    cmake ../ -DCMAKE_INSTALL_PREFIX=${MASON_PREFIX} \
      -DBoost_NO_SYSTEM_PATHS=ON \
      -DTBB_INSTALL_DIR=${MASON_HOME} \
      -DCMAKE_INCLUDE_PATH=${MASON_HOME}/include \
      -DCMAKE_LIBRARY_PATH=${MASON_HOME}/lib \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_EXE_LINKER_FLAGS="${LINK_FLAGS}"
    make -j${MASON_CONCURRENCY} VERBOSE=1
    make install
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_ldflags {
    echo "-L${MASON_PREFIX}/lib -lOSRM"
}

function mason_clean {
    make clean
}

mason_run "$@"

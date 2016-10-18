#!/usr/bin/env bash

set -eu
set -o pipefail

if [[ ${1:-false} == false ]]; then
    echo "please provide install prefix as first arg"
    exit 1
fi

INSTALL_PREFIX=$(realpath $1)

export CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export BUILD_TYPE=${BUILD_TYPE:-Debug}
export JOBS=${JOBS:-2}

# ensure we start inside the osrm-backend directory (one level up)
cd ${CURRENT_DIR}/../

if [[ `which pkg-config` ]]; then
    echo "Success: Found pkg-config";
else
    echo "echo you need pkg-config installed";
    exit 1;
fi;

export MASON_HOME=$(pwd)/mason_packages/.link

function install_build_deps() {
    export CCACHE_VERSION="3.2.4"
    export CMAKE_VERSION="3.5.2"
    export CLANG_VERSION="3.8.0"
    ./.mason/mason install ccache ${CCACHE_VERSION}
    ./.mason/mason install cmake ${CMAKE_VERSION}
    ./.mason/mason install clang ${CLANG_VERSION}
    export PATH=$(./.mason/mason prefix ccache ${CCACHE_VERSION})/bin:${PATH}
    export CMAKE_PATH=$(./.mason/mason prefix cmake ${CMAKE_VERSION})/bin
    export PATH=$(./.mason/mason prefix clang ${CLANG_VERSION})/bin:${PATH}
    export CC=clang-3.8
    export CXX=clang++-3.8
}

function main() {
    export BUILD_DIR="$(pwd)/build"
    if [[ -d "${BUILD_DIR}" ]]; then
        echo "${BUILD_DIR} already exists, please delete before re-running"
        exit 1
    fi
    # setup mason and deps
    ./bootstrap.sh
    source ./scripts/install_node.sh 4
    npm install
    install_build_deps
    export CMAKE_EXTRA_ARGS=""
    if [[ ${AR:-false} != false ]]; then
        export CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DCMAKE_AR=${AR}"
    fi
    if [[ ${RANLIB:-false} != false ]]; then
        export CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DCMAKE_RANLIB=${RANLIB}"
    fi
    if [[ ${NM:-false} != false ]]; then
        export CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DCMAKE_NM=${NM}"
    fi
    mkdir ${BUILD_DIR} && cd ${BUILD_DIR}
    ${CMAKE_PATH}/cmake ../ -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
      -DCMAKE_CXX_COMPILER="${CXX}" \
      -DBoost_NO_SYSTEM_PATHS=ON \
      -DTBB_INSTALL_DIR=${MASON_HOME} \
      -DCMAKE_INCLUDE_PATH=${MASON_HOME}/include \
      -DCMAKE_LIBRARY_PATH=${MASON_HOME}/lib \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -DBoost_USE_STATIC_LIBS=ON \
      -DCMAKE_CXX_FLAGS=-D_GLIBCXX_USE_CXX11_ABI=0 \
      -DBUILD_TOOLS=1 \
      -DENABLE_CCACHE=ON \
      -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS:-OFF} \
      -DCOVERAGE=${COVERAGE:-OFF} \
      ${CMAKE_EXTRA_ARGS}

    make --jobs=${JOBS} VERBOSE=1
    make tests --jobs=${JOBS} VERBOSE=1
    make benchmarks --jobs=${JOBS} VERBOSE=1
    if [[ $(uname -s) == 'Linux' ]]; then
        # make it possible to find libtbb shared libs at custom location
        export LD_LIBRARY_PATH=${MASON_HOME}/lib:${LD_LIBRARY_PATH}
    fi
    # before installing, lets fix the tools and (if it exists) the libosrm shared lib
    # for OS X so they can find the shared libtbb libs
    # on OSX DYLD_LIBRARY_PATH does not work well (not inherited in shells)
    # so to support libraries on custom paths we fix linking using install_name_tool
    if [[ $(uname -s) == 'Darwin' ]]; then
        for tool in $(ls ${BUILD_DIR}/osrm-*); do
          install_name_tool -change libtbb.dylib ${MASON_HOME}/lib/libtbb.dylib ${tool}
          install_name_tool -change libtbbmalloc.dylib ${MASON_HOME}/lib/libtbbmalloc.dylib ${tool}
        done
        if [[ -f ${BUILD_DIR}/libosrm.dylib ]]; then
          install_name_tool -change libtbb.dylib ${MASON_HOME}/lib/libtbb.dylib ${BUILD_DIR}/libosrm.dylib
          install_name_tool -change libtbbmalloc.dylib ${MASON_HOME}/lib/libtbbmalloc.dylib ${BUILD_DIR}/libosrm.dylib
        fi

        # fix the local/non-installed unit tests
        for tool in $(ls ${BUILD_DIR}/unit_tests/*-tests); do
          install_name_tool -change libtbb.dylib ${MASON_HOME}/lib/libtbb.dylib ${tool}
          install_name_tool -change libtbbmalloc.dylib ${MASON_HOME}/lib/libtbbmalloc.dylib ${tool}
        done
    fi

    make install
    export PKG_CONFIG_PATH=${INSTALL_PREFIX}/lib/pkgconfig
    cd ../
    rm -rf example/build
    mkdir -p example/build
    cd example/build
    ${CMAKE_PATH}/cmake ../ -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
    make
    if [[ $(uname -s) == 'Darwin' ]]; then
        # fix the example links
        install_name_tool -change libosrm.dylib ${BUILD_DIR}/libosrm.dylib ./osrm-example
        install_name_tool -change libtbb.dylib ${MASON_HOME}/lib/libtbb.dylib ./osrm-example
        install_name_tool -change libtbbmalloc.dylib ${MASON_HOME}/lib/libtbbmalloc.dylib ./osrm-example
    fi
    cd ../../
    make -C test/data clean || true
    make -C test/data
    make -C test/data benchmark
    ./example/build/osrm-example test/data/monaco.osrm
    cd ${BUILD_DIR}
    ./unit_tests/library-tests ../test/data/monaco.osrm
    ./unit_tests/extractor-tests
    ./unit_tests/engine-tests
    ./unit_tests/util-tests
    ./unit_tests/server-tests
    cd ../
    echo Y | ${BUILD_DIR}/osrm-springclean
    npm test
}

main

set +eu
set +o pipefail
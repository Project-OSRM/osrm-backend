#!/usr/bin/env bash

set -e

INSTALL_PREFIX=$1

if [[ ${INSTALL_PREFIX:-false} == false ]]; then
    echo "please provide install prefix as first arg"
    exit 1
fi

# we pin the mason version to avoid changes in mason breaking builds
MASON_VERSION="3e0cc5a"

if [[ `which pkg-config` ]]; then
    echo "Success: Found pkg-config";
else
    echo "echo you need pkg-config installed";
    exit 1;
fi;

if [[ `which node` ]]; then
    echo "Success: Found node";
else
    echo "echo you need node installed";
    exit 1;
fi;

function dep() {
    ./.mason/mason install $1 $2
    ./.mason/mason link $1 $2
}

function all_deps() {
    dep cmake 3.2.2 &
    dep lua 5.3.0 &
    dep luabind e414c57bcb687bb3091b7c55bbff6947f052e46b &
    dep boost 1.61.0 &
    dep boost_libsystem 1.61.0 &
    dep boost_libthread 1.61.0 &
    dep boost_libfilesystem 1.61.0 &
    dep boost_libprogram_options 1.61.0 &
    dep boost_libregex 1.61.0 &
    dep boost_libiostreams 1.61.0 &
    dep boost_libtest 1.61.0 &
    dep boost_libdate_time 1.61.0 &
    dep expat 2.1.1 &
    dep stxxl 1.4.1 &
    dep bzip2 1.0.6 &
    dep zlib system &
    dep tbb 43_20150316 &
    wait
}

function setup_mason() {
    if [[ ! -d ./.mason ]]; then
        git clone https://github.com/mapbox/mason.git ./.mason
        (cd ./.mason && git checkout ${MASON_VERSION})
    else
        echo "Updating to latest mason"
        (cd ./.mason && git fetch && git checkout ${MASON_VERSION})
    fi
    export MASON_DIR=$(pwd)/.mason
    export MASON_HOME=$(pwd)/mason_packages/.link
    export PATH=$(pwd)/.mason:$PATH
    export CXX=${CXX:-clang++}
    export CC=${CC:-clang}
}


function main() {
    setup_mason
    all_deps
    # put mason installed ccache on PATH
    # then osrm-backend will pick it up automatically
    export CCACHE_VERSION="3.2.4"
    ${MASON_DIR}/mason install ccache ${CCACHE_VERSION}
    export PATH=$(${MASON_DIR}/mason prefix ccache ${CCACHE_VERSION})/bin:${PATH}
    CMAKE_EXTRA_ARGS=""
    if [[ ${AR:-false} != false ]]; then
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DCMAKE_AR=${AR}"
    fi
    if [[ ${RANLIB:-false} != false ]]; then
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DCMAKE_RANLIB=${RANLIB}"
    fi
    if [[ ${NM:-false} != false ]]; then
        CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS} -DCMAKE_NM=${NM}"
    fi
    ${MASON_HOME}/bin/cmake ../ -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
      -DCMAKE_CXX_COMPILER="$CXX" \
      -DBoost_NO_SYSTEM_PATHS=ON \
      -DTBB_INSTALL_DIR=${MASON_HOME} \
      -DCMAKE_INCLUDE_PATH=${MASON_HOME}/include \
      -DCMAKE_LIBRARY_PATH=${MASON_HOME}/lib \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -DBoost_USE_STATIC_LIBS=ON \
      -DBUILD_TOOLS=1 \
      -DENABLE_CCACHE=ON \
      ${CMAKE_EXTRA_ARGS}
}

main
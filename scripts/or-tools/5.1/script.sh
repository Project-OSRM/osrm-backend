#!/usr/bin/env bash

MASON_NAME=or-tools
MASON_VERSION=5.1
MASON_LIB_FILE=lib/libortools.${MASON_DYNLIB_SUFFIX}

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/google/or-tools/archive/v${MASON_VERSION}.tar.gz \
        3d30004e60acfb27776fc7a8d135adb2e1924dde

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/or-tools-${MASON_VERSION}
}

function mason_prepare_compile {
    ${MASON_DIR}/mason install gflags 2.1.2
    MASON_GFLAGS=$(${MASON_DIR}/mason prefix gflags 2.1.2)
    ${MASON_DIR}/mason install protobuf 3.0.0
    MASON_PROTOBUF=$(${MASON_DIR}/mason prefix protobuf 3.0.0)
    ${MASON_DIR}/mason install sparsehash 2.0.2
    MASON_SPARSEHASH=$(${MASON_DIR}/mason prefix sparsehash 2.0.2)
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_ldflags {
    echo "-L${MASON_PREFIX}/lib -lortools -lz"
}

function mason_compile {

    # The following patch to the build script disables some of the more useless
    # and heavyweight parts of the build, like building the automake and autoconf
    # .info docs with TeXinfo.
    patch -N -p1 < ${MASON_DIR}/scripts/${MASON_NAME}/${MASON_VERSION}/patch.diff

    export CXXFLAGS="${CXXFLAGS} -fpermissive"
    export CXX="${MASON_CCACHE} ${CXX}"
    export CC="${MASON_CCACHE} ${CC}"
    export CFLAGS="${CFLAGS} -fpermissive"

    make missing_directories
    mkdir -p $(pwd)/dependencies/install/lib
    mkdir -p $(pwd)/dependencies/install/bin
    mkdir -p $(pwd)/dependencies/install/include/google
    # includes
    ln -s ${MASON_GFLAGS}/include/gflags $(pwd)/dependencies/install/include/gflags
    ln -s ${MASON_PROTOBUF}/include/google/protobuf $(pwd)/dependencies/install/include/google/protobuf
    cp -r ${MASON_SPARSEHASH}/include/google/* $(pwd)/dependencies/install/include/google/
    cp -r ${MASON_SPARSEHASH}/include/sparsehash $(pwd)/dependencies/install/include/
    # programs
    ln -s ${MASON_PROTOBUF}/bin/protoc $(pwd)/dependencies/install/bin/protoc
    # libraries
    ln -s ${MASON_GFLAGS}/lib/libgflags.a $(pwd)/dependencies/install/lib/libgflags.a
    ln -s ${MASON_PROTOBUF}/lib/libprotobuf.a $(pwd)/dependencies/install/lib/libprotobuf.a

    make ortoolslibs -j${MASON_CONCURRENCY}

    if [[ $(uname -s) == "Darwin" ]] ; then
        install_name_tool -id @loader_path/libortools.dylib lib/libortools.dylib
    fi

    mkdir -p ${MASON_PREFIX}/lib/
    cp -r lib/libortools* ${MASON_PREFIX}/lib/

    mkdir -p "${MASON_PREFIX}/include/or-tools/"

    for i in {algorithms,base,bop,constraint_solver,glop,graph,linear_solver,sat,util}; do
        cp -r src/$i ${MASON_PREFIX}/include/or-tools/
    done

    for i in {algorithms,base,bop,constraint_solver,glop,graph,linear_solver,sat,util}; do
        if [[ -d src/gen/$i ]]; then
            cp -r src/gen/$i/*h ${MASON_PREFIX}/include/or-tools/$i/ || true
        fi
    done

}

function mason_static_libs {
    :
}


echo $DIR

mason_run "$@"

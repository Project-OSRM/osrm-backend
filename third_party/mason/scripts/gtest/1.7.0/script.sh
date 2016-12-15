#!/usr/bin/env bash

LIB_VERSION=1.7.0

MASON_NAME=gtest
MASON_VERSION=${LIB_VERSION}
MASON_LIB_FILE=lib/libgtest.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/google/googletest/archive/release-${LIB_VERSION}.tar.gz \
        88afbaea71b4ba50b25f52e13e8c3b01f0f99494

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/googletest-release-${LIB_VERSION}
}

function mason_build {
    mason_load_source
    mason_step "Building for Platform '${MASON_PLATFORM}/${MASON_PLATFORM_VERSION}'..."
    cd "${MASON_BUILD_PATH}"

    mkdir -p ${MASON_PREFIX}/lib
    mkdir -p ${MASON_PREFIX}/include/gtest
    cp -rv include ${MASON_PREFIX}

    rm -rf build
    mkdir -p build
    cd build
    if [ ${MASON_PLATFORM} = 'ios' ]; then
        cmake \
            -GXcode \
            -DCMAKE_TOOLCHAIN_FILE=${MASON_DIR}/utils/ios.cmake \
            ..
        xcodebuild -configuration Release -sdk iphoneos
        xcodebuild -configuration Release -sdk iphonesimulator

        mason_substep "Creating Universal Binary..."
        LIB_FOLDERS="Release-iphoneos Release-iphonesimulator"
        mkdir -p ${MASON_PREFIX}/lib
        for LIB in $(find ${LIB_FOLDERS} -name "*.a" | xargs basename | sort | uniq) ; do
            lipo -create $(find ${LIB_FOLDERS} -name "${LIB}") -output ${MASON_PREFIX}/lib/${LIB}
            lipo -info ${MASON_PREFIX}/lib/${LIB}
        done
    elif [ ${MASON_PLATFORM} = 'android' ]; then
        ${MASON_DIR}/utils/android.sh > toolchain.cmake
        cmake \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX=${MASON_PREFIX} \
            -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake \
            ..
        make VERBOSE=1 -j${MASON_CONCURRENCY} gtest
        cp -v libgtest.a ${MASON_PREFIX}/lib
    else
        cmake \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX=${MASON_PREFIX} \
            ..
        make VERBOSE=1 -j${MASON_CONCURRENCY} gtest
        cp -v libgtest.a ${MASON_PREFIX}/lib
    fi
}

function mason_cflags {
    echo -isystem ${MASON_PREFIX}/include -I${MASON_PREFIX}/include
}

function mason_ldflags {
    if [ ${MASON_PLATFORM} != 'android' ]; then
        echo -lpthread
    fi
}

function mason_static_libs {
    echo ${MASON_PREFIX}/lib/libgtest.a
}


mason_run "$@"

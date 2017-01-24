#!/usr/bin/env bash

MASON_NAME=v8
MASON_VERSION=5.1.281.47
MASON_LIB_FILE=lib/libv8_base.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/${MASON_NAME}/${MASON_NAME}/archive/${MASON_VERSION}.tar.gz \
        3304589e65c878dfe45898abb5e7c7f85a9c9ab6

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}
function mason_prepare_compile {
    CCACHE_VERSION=3.3.1
    ${MASON_DIR}/mason install ccache ${CCACHE_VERSION}
    MASON_CCACHE=$(${MASON_DIR}/mason prefix ccache ${CCACHE_VERSION})

    # create a directory to cache v8 deps
    mkdir -p ../v8-deps
    cd ../v8-deps

    M_GYP_PATH=$(pwd)/build/gyp/
    if [[ ! -d ${M_GYP_PATH} ]]; then
        git clone https://chromium.googlesource.com/external/gyp ${M_GYP_PATH}
        (cd ${M_GYP_PATH} && git checkout 4ec6c4e3a94bd04a6da2858163d40b2429b8aad1)
    fi

    M_ICU_PATH=$(pwd)/third_party/icu/
    if [[ ! -d ${M_ICU_PATH} ]]; then
        git clone https://chromium.googlesource.com/chromium/deps/icu.git ${M_ICU_PATH}
        (cd ${M_ICU_PATH} && git checkout c291cde264469b20ca969ce8832088acb21e0c48)
    fi

    M_BUILDTOOLS_PATH=$(pwd)/buildtools/
    if [[ ! -d ${M_BUILDTOOLS_PATH} ]]; then
        git clone https://chromium.googlesource.com/chromium/buildtools.git ${M_BUILDTOOLS_PATH}
        (cd ${M_BUILDTOOLS_PATH} && git checkout 80b5126f91be4eb359248d28696746ef09d5be67)
    fi
    
    M_CLANG_PATH=$(pwd)/tools/clang/
    if [[ ! -d ${M_CLANG_PATH} ]]; then
        git clone https://chromium.googlesource.com/chromium/src/tools/clang.git ${M_CLANG_PATH}
        (cd ${M_CLANG_PATH} && git checkout faee82e064e04e5cbf60cc7327e7a81d2a4557ad)
    fi
    
    M_TRACE_PATH=$(pwd)/base/trace_event/common/
    if [[ ! -d ${M_TRACE_PATH} ]]; then
        git clone https://chromium.googlesource.com/chromium/src/base/trace_event/common.git ${M_TRACE_PATH}
        (cd ${M_TRACE_PATH} && git checkout c8c8665c2deaf1cc749d9f8e153256d4f67bf1b8)
    fi

    M_GTEST_PATH=$(pwd)/testing/gtest/
    if [[ ! -d ${M_GTEST_PATH} ]]; then
        git clone https://chromium.googlesource.com/external/github.com/google/googletest.git ${M_GTEST_PATH}
        (cd ${M_GTEST_PATH} && git checkout 6f8a66431cb592dad629028a50b3dd418a408c87)
    fi

    M_GMOCK_PATH=$(pwd)/testing/gmock/
    if [[ ! -d ${M_GMOCK_PATH} ]]; then
        git clone https://chromium.googlesource.com/external/googlemock.git ${M_GMOCK_PATH}
        (cd ${M_GMOCK_PATH} && git checkout 0421b6f358139f02e102c9c332ce19a33faf75be)
    fi

    M_SWARMING_PATH=$(pwd)/tools/swarming_client/
    if [[ ! -d ${M_SWARMING_PATH} ]]; then
        git clone https://chromium.googlesource.com/external/swarming.client.git ${M_SWARMING_PATH}
        (cd ${M_SWARMING_PATH} && git checkout df6e95e7669883c8fe9ef956c69a544154701a49)
    fi
    
}

function mason_compile {
    if [[ $(uname -s) == 'Darwin' ]]; then
        export LDFLAGS="${LDFLAGS:-} -stdlib=libc++"
    fi
    export CXX="${MASON_CCACHE}/bin/ccache ${CXX}"
    cp -r ${M_GYP_PATH}         build/gyp
    cp -r ${M_ICU_PATH}         third_party/icu
    cp -r ${M_BUILDTOOLS_PATH}  buildtools
    cp -r ${M_CLANG_PATH}       tools/clang
    mkdir -p base/trace_event
    cp -r ${M_TRACE_PATH}       base/trace_event/common
    cp -r ${M_GTEST_PATH}       testing/gtest
    cp -r ${M_GMOCK_PATH}       testing/gmock
    cp -r ${M_SWARMING_PATH}    tools/swarming_client

    # make gyp default to full archives for static libs rather than thin
    if [[ $(uname -s) == 'Linux' ]]; then
        perl -i -p -e "s/\'standalone_static_library\', 0\)/\'standalone_static_library\', 1\)/g;" build/gyp/pylib/gyp/generator/make.py
    fi
    #PYTHONPATH="$(pwd)/tools/generate_shim_headers:$(pwd)/build:$(pwd)/build/gyp/pylib:" \
    #GYP_GENERATORS=make \
    #build/gyp/gyp --generator-output="out" build/all.gyp \
    #              -Ibuild/standalone.gypi --depth=. \
    #              -Dv8_target_arch=x64 \
    #              -Dtarget_arch=x64 \
    #              -S.x64.release  -Dv8_enable_backtrace=1 -Dwerror='' -Darm_fpu=default -Darm_float_abi=default \
    #              -Dv8_no_strict_aliasing=1 -Dv8_enable_i18n_support=0
    GYPFLAGS=-Dmac_deployment_target=10.8 make x64.release library=static werror=no snapshot=on  strictaliasing=off i18nsupport=off -j${MASON_CONCURRENCY}
    mkdir -p ${MASON_PREFIX}/include
    mkdir -p ${MASON_PREFIX}/lib
    cp -r include/* ${MASON_PREFIX}/include/
    if [[ $(uname -s) == 'Darwin' ]]; then
      cp out/x64.release/*v8*.a ${MASON_PREFIX}/lib/
    else
      cp out/x64.release/obj.target/tools/gyp/lib*.a ${MASON_PREFIX}/lib/
    fi
    strip -S ${MASON_PREFIX}/lib/*
}

function mason_cflags {
    echo -I${MASON_PREFIX}/include
}

function mason_ldflags {
    :
}

function mason_static_libs {
    echo ${MASON_PREFIX}/lib/libv8_base.a ${MASON_PREFIX}/lib/libv8_libplatform.a ${MASON_PREFIX}/lib/libv8_external_snapshot.a ${MASON_PREFIX}/lib/libv8_libbase.a
}

function mason_clean {
    make clean
}

mason_run "$@"

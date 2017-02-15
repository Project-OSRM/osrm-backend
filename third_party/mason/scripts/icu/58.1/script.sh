#!/usr/bin/env bash

# Build ICU common package (libicuuc.a) with data file separate and with support for legacy conversion and break iteration turned off in order to minimize size

MASON_NAME=icu
MASON_VERSION=58.1
MASON_LIB_FILE=lib/libicuuc.a
#MASON_PKGCONFIG_FILE=lib/pkgconfig/icu-uc.pc

. ${MASON_DIR}/mason.sh

MASON_BUILD_DEBUG=0 # Enable to build library with debug symbols
MASON_CROSS_BUILD=0

function mason_load_source {
    mason_download \
        http://download.icu-project.org/files/icu4c/58.1/icu4c-58_1-src.tgz \
        ad6995ba349ed79dde0f25d125a9b0bb56979420

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}
}

function mason_prepare_compile {
    if [[ ${MASON_PLATFORM} == 'ios' || ${MASON_PLATFORM} == 'android' || ${MASON_PLATFORM_VERSION} != `uname -m` ]]; then
        mason_substep "Cross-compiling ICU. Starting with host build of ICU to generate tools."

        pushd ${MASON_ROOT}/..
        env -i HOME="$HOME" PATH="$PATH" USER="$USER" ${MASON_DIR}/mason build icu ${MASON_VERSION}
        popd

        # TODO: Copies a bunch of files to a kind of orphaned place, do we need to do something to clean up after the build?
        #  Copying the whole build directory is the easiest way to do a cross build, but we could limit this to a small subset of files (icucross.mk, the tools directory, probably a few others...)
        #  Also instead of using the regular build steps, we could use a dedicated built target that just builds the tools
        mason_substep "Moving host ICU build directory to ${MASON_ROOT}/.build/icu-host"
        rm -rf ${MASON_ROOT}/.build/icu-host
        cp -R ${MASON_BUILD_PATH}/source ${MASON_ROOT}/.build/icu-host
    fi
}

function mason_compile {
    if [[ ${MASON_PLATFORM} == 'ios' || ${MASON_PLATFORM} == 'android' || ${MASON_PLATFORM_VERSION} != `uname -m` ]]; then
        MASON_CROSS_BUILD=1
    fi
    mason_compile_base
}

function mason_compile_base {
    pushd  ${MASON_BUILD_PATH}/source

    # Using uint_least16_t instead of char16_t because Android Clang doesn't recognize char16_t
    # I'm being shady and telling users of the library to use char16_t, so there's an implicit raw cast
    ICU_CORE_CPP_FLAGS="-DU_CHARSET_IS_UTF8=1 -DU_CHAR_TYPE=uint_least16_t"
    ICU_MODULE_CPP_FLAGS="${ICU_CORE_CPP_FLAGS} -DUCONFIG_NO_LEGACY_CONVERSION=1 -DUCONFIG_NO_BREAK_ITERATION=1"
    
    CPPFLAGS="${CPPFLAGS} ${ICU_CORE_CPP_FLAGS} ${ICU_MODULE_CPP_FLAGS} -fvisibility=hidden $(icu_debug_cpp)"
    #CXXFLAGS="--std=c++0x"

    echo "Configuring with ${MASON_HOST_ARG}"

    ./configure ${MASON_HOST_ARG} --prefix=${MASON_PREFIX} \
    $(icu_debug_configure) \
    $(cross_build_configure) \
    --with-data-packaging=archive \
    --enable-renaming \
    --enable-strict \
    --enable-static \
    --enable-draft \
    --disable-rpath \
    --disable-shared \
    --disable-tests \
    --disable-extras \
    --disable-tracing \
    --disable-layout \
    --disable-icuio \
    --disable-samples \
    --disable-dyload || cat config.log


    # Must do make clean after configure to clear out object files left over from previous build on different architecture
    make clean
    make -j${MASON_CONCURRENCY}
    make install
    popd
}

function icu_debug_cpp {
    if [ ${MASON_BUILD_DEBUG} ]; then
        echo "-glldb"
    fi
}

function icu_debug_configure {
    if [ ${MASON_BUILD_DEBUG} == 1 ]; then
        echo "--enable-debug --disable-release"
    else
        echo "--enable-release --disable-debug"
    fi
}

function cross_build_configure {
    # Building tools is disabled in cross-build mode. Using the host-built version of the tools is the whole point of the --with-cross-build flag
    if [ ${MASON_CROSS_BUILD} == 1 ]; then
        echo "--with-cross-build=${MASON_ROOT}/.build/icu-host --disable-tools"
    else
        echo "--enable-tools"
    fi
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include -DUCHAR_TYPE=char16_t"
}

function mason_ldflags {
    echo ""
}

mason_run "$@"

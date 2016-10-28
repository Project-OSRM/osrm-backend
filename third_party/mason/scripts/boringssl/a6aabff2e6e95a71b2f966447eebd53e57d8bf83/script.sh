#!/usr/bin/env bash

MASON_NAME=boringssl
MASON_VERSION=a6aabff2e6e95a71b2f966447eebd53e57d8bf83
MASON_LIB_FILE=lib/libboringssl.a

. ${MASON_DIR}/mason.sh

MASON_PWD=$(pwd)

function mason_load_source {
    # get gyp build scripts
    URL=https://chromium.googlesource.com/experimental/chromium/src/+archive/master/third_party/boringssl.tar.gz
    # we don't use `mason_download` here because the hash changes every download (google must be generating on the fly)
    mkdir -p "${MASON_ROOT}/.cache"
    cd "${MASON_ROOT}/.cache"
    if [ ! -f ${MASON_SLUG} ] ; then
        mason_step "Downloading ${URL}..."
        curl --retry 3 -f -# -L "${URL}" -o ${MASON_SLUG}
    fi
    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/
}

function mason_compile {
    # get code
    git clone --depth 1 https://boringssl.googlesource.com/boringssl src
    # get gyp
    git clone --depth 1 https://chromium.googlesource.com/external/gyp.git

    # TODO - download this patch from remote to be able to work non-locally
    patch ./update_gypi_and_asm.py < ${MASON_PWD}/boringssl_asm_x86_64_fPIC.diff
    # regenerate gyp configs
    python update_gypi_and_asm.py

    if [[ "${MASON_PLATFORM}" == "android" ]]; then
        if [[ "${MASON_ANDROID_ARCH}" == "arm" ]]; then
            export GYP_DEFINES="component=static_library OS=android target_arch=arm"
        elif [[ "${MASON_ANDROID_ARCH}" == "x86" ]]; then
            export GYP_DEFINES="component=static_library OS=android target_arch=ia32"
        else
            # Note: mips will be arch "mipsel"
            export GYP_DEFINES="component=static_library OS=android target_arch=${MASON_ANDROID_ARCH}"
        fi
    else
        export GYP_DEFINES="component=static_library target_arch=`uname -m | sed -e "s/i.86/ia32/;s/x86_64/x64/;s/amd64/x64/;s/arm.*/arm/;s/i86pc/ia32/"`"
    fi

    # generate makefiles
    echo "{ 'target_defaults': { 'standalone_static_library': 1 } }" > config.gypi
    ./gyp/gyp boringssl.gyp --depth=. -Iconfig.gypi --generator-output=./build --format=make -Dcomponent=static_library -Dtarget_arch=x64
    
    # compile
    make -j${MASON_CONCURRENCY} -C build V=1

    # install
    mkdir -p ${MASON_PREFIX}/lib
    if [[ "${MASON_PLATFORM}" == "osx" ]]; then
        cp build/out/Default/libboringssl.a ${MASON_PREFIX}/lib/libboringssl.a
    else
        cp build/out/Default/obj.target/libboringssl.a ${MASON_PREFIX}/lib/libboringssl.a
    fi
    (cd ${MASON_PREFIX}/lib/ && ln -s libboringssl.a libssl.a && ln -s libboringssl.a libcrypto.a)
    cp -r src/include ${MASON_PREFIX}/include
}

function mason_cflags {
    echo -I${MASON_PREFIX}/include
}

function mason_ldflags {
    echo -L${MASON_PREFIX}/lib -lboringssl
}

function mason_clean {
    make clean
}

mason_run "$@"

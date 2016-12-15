#!/usr/bin/env bash

set -u

CODE=0

function check_cflags() {
    MASON_CFLAGS=$(./mason cflags ${MASON_NAME} ${MASON_VERSION})
    MASON_CFLAGS=${MASON_CFLAGS/-I/}
    if [[ ! -d ${MASON_CFLAGS} ]]; then
        echo "not ok: Path for cflags not found: ${MASON_CFLAGS}"
        CODE=1
    else
        echo "ok: path to cflags found: ${MASON_CFLAGS}"
    fi
}

function check_ldflags() {
    MASON_LDFLAGS=$(./mason ldflags ${MASON_NAME} ${MASON_VERSION})
    for var in $MASON_LDFLAGS; do
        if [[ "${var}" =~ "-L" ]]; then
            vpath=${var/-L/}
            if [[ ! -d ${vpath} ]]; then
                echo "not ok: Path for ldflags not found: ${vpath}"
                CODE=1
            else
                echo "ok: path to ldflags found: ${vpath}"
            fi
        fi
    done
}

function read_link() {
    # number of processors on the current system
    case "$(uname -s)" in
        'Linux')    readlink -f $1;;
        'Darwin')   readlink $1;;
        *)          echo 1;;
    esac
}

# symlinks should be two deep
function check_file_links() {
    if [[ ! -L ./mason_packages/.link/$1 ]]; then
        echo "not ok: ./mason_packages/.link/$1 is not a symlink"
        CODE=1
    else
        resolved=$(read_link ./mason_packages/.link/$1)
        echo "ok: $resolved is a symlink"
        # resolve osx symlinks further
        if [[ -L $resolved ]]; then
            resolved=$(read_link $resolved)
        fi
        if [[ ! -f $resolved ]]; then
            echo "not ok: $resolved is not a file"
            CODE=1
        else
            echo "ok: $resolved is a file"
            expected_keyword=""
            if [[ ${MASON_PLATFORM} == 'osx' ]]; then
                expected_keyword="MacOSX.platform"
            elif [[ ${MASON_PLATFORM} == 'linux' ]]; then
                if [[ ${1} =~ "libz" ]]; then
                    expected_keyword="/lib/x86_64-linux-gnu/"
                elif [[ ${1} =~ "include" ]]; then
                    expected_keyword="/usr/include/"
                fi
            elif [[ ${MASON_PLATFORM} == 'android' ]]; then
                MASON_ANDROID_ABI=$(${MASON_DIR}/mason env MASON_ANDROID_ABI)
                MASON_NDK_PACKAGE_VERSION=$(${MASON_DIR}/mason env MASON_NDK_PACKAGE_VERSION)
                expected_keyword="android-ndk/${MASON_NDK_PACKAGE_VERSION}"
            fi
            if [[ "$resolved" =~ "${expected_keyword}" ]]; then
                echo "ok: '${expected_keyword}' found in path $resolved"
            else
                echo "not ok: '${expected_keyword}' not found in path $resolved"
                CODE=1
            fi
        fi
    fi
}

function check_shared_lib_info() {
    resolved=$(read_link ./mason_packages/.link/$1)
    if [[ -f $resolved ]]; then
        echo "ok: resolved to $resolved"
        if [[ ${MASON_PLATFORM} == 'osx' ]]; then
            file $resolved
            otool -L $resolved
            lipo -info $resolved
        elif [[ ${MASON_PLATFORM} == 'linux' ]]; then
            file $resolved
            ldd $resolved
            readelf -d $resolved
        elif [[ ${MASON_PLATFORM} == 'android' ]]; then
            FILE_DETAILS=$(file $resolved)
            MASON_ANDROID_ARCH=$(${MASON_DIR}/mason env MASON_ANDROID_ARCH)
            if [[ ${MASON_ANDROID_ARCH} =~ "64" ]]; then
                if [[ ${FILE_DETAILS} =~ "64-bit" ]]; then
                    echo "ok: ${MASON_ANDROID_ARCH} 64-bit arch (for ${MASON_ANDROID_ABI}) expected and detected in $FILE_DETAILS"
                else
                    echo "not ok: ${MASON_ANDROID_ARCH} 64-bit arch (for ${MASON_ANDROID_ABI}) expected and not detected in $FILE_DETAILS"
                    CODE=1
                fi
            else
                if [[ ${FILE_DETAILS} =~ "64-bit" ]]; then
                    echo "not ok: ${MASON_ANDROID_ARCH} 32 bit arch (for ${MASON_ANDROID_ABI}) expected and not detected in $FILE_DETAILS"
                    CODE=1
                fi
            fi
            BIN_PATH=$(${MASON_DIR}/mason env MASON_SDK_ROOT)/bin
            MASON_ANDROID_TOOLCHAIN=$(${MASON_DIR}/mason env MASON_ANDROID_TOOLCHAIN)
            ${BIN_PATH}/${MASON_ANDROID_TOOLCHAIN}-readelf -d $resolved
        fi

    else
        echo "not okay: could not resolve to file: $resolved"
        CODE=1
    fi
}

if [[ $MASON_PLATFORM != 'ios' ]]; then
    check_cflags
    check_ldflags
    check_file_links "include/zlib.h"
    check_file_links "include/zconf.h"
    check_file_links "lib/libz.$(${MASON_DIR}/mason env MASON_DYNLIB_SUFFIX)"
    if [[ ${CODE} == 0 ]]; then
        check_shared_lib_info "lib/libz.$(${MASON_DIR}/mason env MASON_DYNLIB_SUFFIX)"
    else
        echo "Error already occured so skipping shared library test"
    fi
fi

exit ${CODE}

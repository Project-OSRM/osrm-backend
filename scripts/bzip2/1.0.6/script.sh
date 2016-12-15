#!/usr/bin/env bash

MASON_NAME=bzip2
MASON_VERSION=1.0.6
MASON_LIB_FILE=lib/libbz2.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://www.bzip.org/1.0.6/bzip2-1.0.6.tar.gz \
        e47e9034c4116f467618cfaaa4d3aca004094007

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function read_link() {
    case "$(uname -s)" in
        'Linux')    readlink -f $1;;
        'Darwin')   readlink $1;;
        *)          echo 1;;
    esac
}

function mason_compile {
    make install PREFIX=${MASON_PREFIX} CC="$CC" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS"
    # symlinks are not portable, so now we recurse into /bin directory
    # and fix them to be portable by being relative
    cd ${MASON_PREFIX}/bin
    for i in $(ls *); do
        if [[ -L $i ]]; then
            ln -sf $(basename $(read_link $i)) $i
        fi
    done
    # TODO: android may need ranlib manual call
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_ldflags {
    echo "-L${MASON_PREFIX}/lib -lbz2"
}

function mason_clean {
    make clean
}

mason_run "$@"

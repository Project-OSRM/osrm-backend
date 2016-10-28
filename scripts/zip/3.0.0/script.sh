#!/usr/bin/env bash

MASON_NAME=zip
MASON_VERSION=3.0.0
MASON_LIB_FILE=bin/zip

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://sourceforge.net/projects/infozip/files/Zip%203.x%20%28latest%29/3.0/zip30.tar.gz/download \
        57f60be499bef90ccf84fe47d522d32504609e9b

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/zip30
}

function mason_compile {
    perl -i -p -e "s/prefix = \/usr\/local/prefix = ${MASON_PREFIX//\//\\/}/g;" unix/Makefile
    make -f unix/Makefile install
}

function mason_clean {
    make -f unix/Makefile clean
}

mason_run "$@"

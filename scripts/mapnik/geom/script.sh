#!/usr/bin/env bash

MASON_NAME=mapnik
MASON_VERSION=geom
MASON_LIB_FILE=lib/libmapnik-wkt.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    export MASON_BUILD_PATH=${MASON_ROOT}/.build/mapnik-3.x
    if [[ ! -d ${MASON_BUILD_PATH} ]]; then
        git clone https://github.com/mapnik/mapnik.git ${MASON_BUILD_PATH} -b mapnik-geometry
    else
        (cd ${MASON_BUILD_PATH} && git pull && git checkout mapnik-geometry)
    fi
}

function mason_compile {
    echo $(pwd)
    source bootstrap.sh
    cat config.py
    ./configure PREFIX=${MASON_PREFIX} PYTHON_PREFIX=${MASON_PREFIX} PATH_REPLACE=''
    cat config.py
    echo $(pwd)
    JOBS=${MASON_CONCURRENCY} make
    make install
    # push over GDAL_DATA, ICU_DATA, PROJ_LIB
    # fix mapnik-config entries for deps
    HERE=$(pwd)
    python -c "data=open('$MASON_PREFIX/bin/mapnik-config','r').read();open('$MASON_PREFIX/bin/mapnik-config','w').write(data.replace('$HERE','.'))"
    cat $MASON_PREFIX/bin/mapnik-config
    mkdir -p ${MASON_PREFIX}/share/gdal
    mkdir -p ${MASON_PREFIX}/share/proj
    mkdir -p ${MASON_PREFIX}/share/icu
    cp -r $GDAL_DATA/ ${MASON_PREFIX}/share/gdal/
    cp -r $PROJ_LIB/ ${MASON_PREFIX}/share/proj/
    cp -r $ICU_DATA/*dat ${MASON_PREFIX}/share/icu/
    find ${MASON_PREFIX} -name "*.pyc" -exec rm {} \;
}

function mason_cflags {
    ""
}

function mason_ldflags {
    ""
}

function mason_clean {
    make clean
}

mason_run "$@"

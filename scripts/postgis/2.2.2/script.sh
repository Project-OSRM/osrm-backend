#!/usr/bin/env bash

MASON_NAME=postgis
MASON_VERSION=2.2.2
MASON_LIB_FILE=bin/shp2pgsql

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://download.osgeo.org/postgis/source/postgis-${MASON_VERSION}.tar.gz \
        e3a740fc6d9af5d567346f2729ee86af2b6da88c

    mason_extract_tar_gz
    export MASON_BUILD_PATH=${MASON_ROOT}/.build/postgis-${MASON_VERSION}
}

function mason_prepare_compile {
    cd $(dirname ${MASON_ROOT})
    ${MASON_DIR}/mason install postgres 9.5.2
    MASON_POSTGRES=$(${MASON_DIR}/mason prefix postgres 9.5.2)
    ${MASON_DIR}/mason install proj 4.9.2
    MASON_PROJ=$(${MASON_DIR}/mason prefix proj 4.9.2)
    ${MASON_DIR}/mason install libxml2 2.9.3
    MASON_XML2=$(${MASON_DIR}/mason prefix libxml2 2.9.3)
    ${MASON_DIR}/mason install geos 3.5.0
    MASON_GEOS=$(${MASON_DIR}/mason prefix geos 3.5.0)
    if [[ $(uname -s) == 'Darwin' ]]; then
        FIND="\/Users\/travis\/build\/mapbox\/mason"
    else
        FIND="\/home\/travis\/build\/mapbox\/mason"
    fi
    REPLACE="$(pwd)"
    REPLACE=${REPLACE////\\/}
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_PROJ}/lib/libproj.la
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_XML2}/lib/libxml2.la
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_XML2}/bin/xml2-config
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_GEOS}/lib/libgeos.la
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_GEOS}/lib/libgeos_c.la
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_GEOS}/bin/geos-config

    ${MASON_DIR}/mason install gdal 2.0.2
    MASON_GDAL=$(${MASON_DIR}/mason prefix gdal 2.0.2)
    ln -sf ${MASON_GDAL}/include ${MASON_GDAL}/include/gdal
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_GDAL}/lib/libgdal.la
    ${MASON_DIR}/mason install libtiff 4.0.6
    MASON_TIFF=$(${MASON_DIR}/mason prefix libtiff 4.0.6)
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_TIFF}/lib/libtiff.la
    ${MASON_DIR}/mason install jpeg_turbo 1.4.2
    MASON_JPEG=$(${MASON_DIR}/mason prefix jpeg_turbo 1.4.2)
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_JPEG}/lib/libjpeg.la
    ${MASON_DIR}/mason install libpng 1.6.21
    MASON_PNG=$(${MASON_DIR}/mason prefix libpng 1.6.21)
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_PNG}/lib/libpng.la
    ${MASON_DIR}/mason install expat 2.1.1
    MASON_EXPAT=$(${MASON_DIR}/mason prefix expat 2.1.1)
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_EXPAT}/lib/libexpat.la
    ${MASON_DIR}/mason install zlib system
    MASON_ZLIB=$(${MASON_DIR}/mason prefix zlib system)
    #${MASON_DIR}/mason install iconv system
    #MASON_ICONV=$(${MASON_DIR}/mason prefix iconv system)
}

function mason_compile {
    export LDFLAGS="${LDFLAGS} \
      -L${MASON_GDAL}/lib -lgdal \
      -L${MASON_GEOS}/lib -lgeos_c -lgeos\
      -L${MASON_ZLIB}/lib -lz \
      -L${MASON_TIFF}/lib -ltiff \
      -L${MASON_JPEG}/lib -ljpeg \
      -L${MASON_PROJ}/lib -lproj \
      -L${MASON_PNG}/lib -lpng \
      -L${MASON_EXPAT}/lib -lexpat \
      -L${MASON_PROJ}/lib -lproj \
      -L${MASON_XML2}/lib -lxml2"
    export CFLAGS="${CFLAGS} -I$(pwd)/liblwgeom/ \
      -I$(pwd)/raster/ -I$(pwd)/raster/rt_core/ \
      -I${MASON_TIFF}/include \
      -I${MASON_JPEG}/include \
      -I${MASON_PROJ}/include \
      -I${MASON_PNG}/include \
      -I${MASON_EXPAT}/include \
      -I${MASON_GDAL}/include \
      -I${MASON_POSTGRES}/include/server \
      -I${MASON_GEOS}/include \
      -I${MASON_PROJ}/include \
      -I${MASON_XML2}/include/libxml2"

    if [[ $(uname -s) == 'Darwin' ]]; then
        export LDFLAGS="${LDFLAGS} -Wl,-lc++ -Wl,${MASON_GDAL}/lib/libgdal.a -Wl,${MASON_POSTGRES}/lib/libpq.a -liconv"
    else
        export LDFLAGS="${LDFLAGS} ${MASON_GDAL}/lib/libgdal.a -lgeos_c -lgeos -lxml2 -lproj -lexpat -lpng -ltiff -ljpeg ${MASON_POSTGRES}/lib/libpq.a -pthread -ldl -lz -lstdc++ -lm"
    fi


    MASON_LIBPQ_PATH=${MASON_POSTGRES}/lib/libpq.a
    MASON_LIBPQ_PATH2=${MASON_LIBPQ_PATH////\\/}
    perl -i -p -e "s/\-lpq/${MASON_LIBPQ_PATH2} -pthread/g;" configure
    perl -i -p -e "s/librtcore\.a/librtcore\.a \.\.\/\.\.\/liblwgeom\/\.libs\/liblwgeom\.a/g;" raster/loader/Makefile.in

    if [[ $(uname -s) == 'Linux' ]]; then
      # help initGEOS configure check
      perl -i -p -e "s/\-lgeos_c  /\-lgeos_c \-lgeos \-lstdc++ \-lm /g;" configure
      # help GDALAllRegister configure check
      CMD="data=open('./configure','r').read();open('./configure','w')"
      CMD="${CMD}.write(data.replace('\`\$GDAL_CONFIG --libs\`','\"-lgdal -lgeos_c -lgeos -lxml2 -lproj -lexpat -lpng -ltiff -ljpeg ${MASON_POSTGRES}/lib/libpq.a -pthread -ldl -lz -lstdc++ -lm\"'))"
      python -c "${CMD}"
    fi

    ./configure \
        --enable-static --disable-shared \
        --prefix=$(mktemp -d) \
        ${MASON_HOST_ARG} \
        --with-projdir=${MASON_PROJ} \
        --with-geosconfig=${MASON_GEOS}/bin/geos-config \
        --with-pgconfig=${MASON_POSTGRES}/bin/pg_config \
        --with-xml2config=${MASON_XML2}/bin/xml2-config \
        --with-gdalconfig=${MASON_GDAL}/bin/gdal-config \
        --without-json \
        --without-gui \
        --with-topology \
        --with-raster \
        --with-sfcgal=no \
        --without-sfcgal \
        --disable-nls || (cat config.log && exit 1)
    # -j${MASON_CONCURRENCY} disabled due to https://trac.osgeo.org/postgis/ticket/3345
    make LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS"
    make install LDFLAGS="$LDFLAGS" CFLAGS="$CFLAGS"
    # the meat of postgis installs into postgres directory
    # so we actually want to package postgres with the postgis stuff
    # inside, so here we symlink it
    mkdir -p $(dirname $MASON_PREFIX)
    ln -sf ${MASON_POSTGRES} ${MASON_PREFIX}
}

function mason_clean {
    make clean
}

mason_run "$@"

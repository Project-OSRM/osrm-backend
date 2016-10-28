#!/usr/bin/env bash

MASON_NAME=gdal
MASON_VERSION=1.11.2
MASON_LIB_FILE=lib/libgdal.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://download.osgeo.org/gdal/${MASON_VERSION}/gdal-${MASON_VERSION}.tar.gz \
        50660f82fb01ff1c97f6342a3fbbe5bdc6d01b09

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_prepare_compile {
    cd $(dirname ${MASON_ROOT})
    # set up to fix libtool .la files
    # https://github.com/mapbox/mason/issues/61
    FIND="\/Users\/travis\/build\/mapbox\/mason"
    REPLACE="$(pwd)"
    REPLACE=${REPLACE////\\/}
    ${MASON_DIR}/mason install libtiff 4.0.4beta
    MASON_TIFF=$(${MASON_DIR}/mason prefix libtiff 4.0.4beta)
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_TIFF}/lib/libtiff.la
    ${MASON_DIR}/mason install proj 4.8.0
    MASON_PROJ=$(${MASON_DIR}/mason prefix proj 4.8.0)
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_PROJ}/lib/libproj.la
    ${MASON_DIR}/mason install jpeg_turbo 1.4.0
    MASON_JPEG=$(${MASON_DIR}/mason prefix jpeg_turbo 1.4.0)
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_JPEG}/lib/libjpeg.la
    ${MASON_DIR}/mason install libpng 1.6.20
    MASON_PNG=$(${MASON_DIR}/mason prefix libpng 1.6.20)
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_PNG}/lib/libpng.la
    ${MASON_DIR}/mason install expat 2.1.0
    MASON_EXPAT=$(${MASON_DIR}/mason prefix expat 2.1.0)
    perl -i -p -e "s/${FIND}/${REPLACE}/g;" ${MASON_EXPAT}/lib/libexpat.la
    # depends on sudo apt-get install zlib1g-dev
    ${MASON_DIR}/mason install zlib system
    MASON_ZLIB=$(${MASON_DIR}/mason prefix zlib system)
}

function mason_compile {
    mason_step "Loading patch 'https://github.com/mapbox/mason/blob/${MASON_SLUG}/patch.diff'..."
    curl --retry 3 -s -f -# -L \
      https://raw.githubusercontent.com/mapbox/mason/${MASON_SLUG}/patch.diff \
      -O || (mason_error "Could not find patch for ${MASON_SLUG}" && exit 1)
    patch -N -p1 < ./patch.diff
    CUSTOM_LIBS="-L${MASON_TIFF}/lib -ltiff -L${MASON_JPEG}/lib -ljpeg -L${MASON_PROJ}/lib -lproj -L${MASON_PNG}/lib -lpng -L${MASON_EXPAT}/lib -lexpat"
    CUSTOM_CFLAGS="${CFLAGS} -I${MASON_LIBPQ}/include -I${MASON_TIFF}/include -I${MASON_JPEG}/include -I${MASON_PROJ}/include -I${MASON_PNG}/include -I${MASON_EXPAT}/include"
    CUSTOM_LDFLAGS="${LDFLAGS}"
    # note: it might be tempting to build with --without-libtool
    # but I find that will only lead to a static libgdal.a and will
    # not produce a shared library no matter if --enable-shared is passed

    # note: we put ${STDLIB_CXXFLAGS} into CXX instead of LDFLAGS due to libtool oddity:
    # http://stackoverflow.com/questions/16248360/autotools-libtool-link-library-with-libstdc-despite-stdlib-libc-option-pass
    if [[ $(uname -s) == 'Darwin' ]]; then
        CXX="${CXX} -stdlib=libc++ -std=c++11"
    fi

    LIBS="${CUSTOM_LIBS}" LDFLAGS="${CUSTOM_LDFLAGS}" CFLAGS="${CUSTOM_CFLAGS}" ./configure \
        --enable-static --disable-shared \
        ${MASON_HOST_ARG} \
        --prefix=${MASON_PREFIX} \
        --with-libz=/usr/ \
        --disable-rpath \
        --with-libjson-c=internal \
        --with-geotiff=internal \
        --with-expat=${MASON_EXPAT} \
        --with-threads=yes \
        --with-fgdb=no \
        --with-rename-internal-libtiff-symbols=no \
        --with-rename-internal-libgeotiff-symbols=no \
        --with-hide-internal-symbols=yes \
        --with-libtiff=${MASON_TIFF} \
        --with-jpeg=${MASON_JPEG} \
        --with-png=${MASON_PNG} \
        --with-static-proj4=${MASON_PROJ} \
        --with-spatialite=no \
        --with-geos=no \
        --with-sqlite3=no \
        --with-curl=no \
        --with-xml2=no \
        --with-pcraster=no \
        --with-cfitsio=no \
        --with-odbc=no \
        --with-libkml=no \
        --with-pcidsk=no \
        --with-jasper=no \
        --with-gif=no \
        --with-pg=no \
        --with-grib=no \
        --with-freexl=no \
        --with-avx=no \
        --with-sse=no \
        --with-perl=no \
        --with-ruby=no \
        --with-python=no \
        --with-java=no \
        --with-podofo=no \
        --without-pam \
        --with-webp=no \
        --with-pcre=no \
        --with-lzma=no

    make -j${MASON_CONCURRENCY}
    make install

    # attempt to make paths relative in gdal-config
    python -c "data=open('$MASON_PREFIX/bin/gdal-config','r').read();open('$MASON_PREFIX/bin/gdal-config','w').write(data.replace('$MASON_PREFIX','\$( cd \"\$( dirname \$( dirname \"\$0\" ))\" && pwd )'))"
    cat $MASON_PREFIX/bin/gdal-config
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include/gdal"
}

function mason_ldflags {
    echo $(${MASON_PREFIX}/bin/gdal-config --static --libs)
}

function mason_clean {
    make clean
}

mason_run "$@"

#!/usr/bin/env bash

MASON_NAME=gdal
MASON_VERSION=dev
MASON_LIB_FILE=lib/libgdal.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    export MASON_BUILD_PATH=${MASON_ROOT}/.build/gdal-2.x
    if [[ ! -d ${MASON_BUILD_PATH} ]]; then
        git clone --depth 1 https://github.com/springmeyer/gdal.git -b build-fixes ${MASON_BUILD_PATH}
    else
        (cd ${MASON_BUILD_PATH} && git pull)
    fi
}

if [[ $(uname -s) == 'Darwin' ]]; then
    FIND_PATTERN="\/Users\/travis\/build\/mapbox\/mason"
else
    FIND_PATTERN="\/home\/travis\/build\/mapbox\/mason"
fi

function install_dep {
    # set up to fix libtool .la files
    # https://github.com/mapbox/mason/issues/61
    REPLACE="$(pwd)"
    REPLACE=${REPLACE////\\/}
    ${MASON_DIR}/mason install $1 $2
    ${MASON_DIR}/mason link $1 $2
    LA_FILE=$(${MASON_DIR}/mason prefix $1 $2)/lib/$3.la
    if [[ -f ${LA_FILE} ]]; then
       perl -i -p -e "s/${FIND_PATTERN}/${REPLACE}/g;" ${LA_FILE}
    else
        echo "$LA_FILE not found"
    fi
}

function mason_prepare_compile {
    cd $(dirname ${MASON_ROOT})
    install_dep libtiff 4.0.4beta libtiff
    install_dep proj 4.8.0 libproj
    install_dep jpeg_turbo 1.4.0 libjpeg
    install_dep libpng 1.6.16 libpng
    install_dep expat 2.1.0 libexpat
    install_dep libpq 9.4.0 libpq
    # depends on sudo apt-get install zlib1g-dev
    ${MASON_DIR}/mason install zlib system
    MASON_ZLIB=$(${MASON_DIR}/mason prefix zlib system)
    # depends on sudo apt-get install libc6-dev
    #${MASON_DIR}/mason install iconv system
    #MASON_ICONV=$(${MASON_DIR}/mason prefix iconv system)
}

function mason_compile {
    LINK_DIR="${MASON_ROOT}/.link"
    echo $LINK_DIR
    export LIBRARY_PATH=${LINK_DIR}/lib:${LIBRARY_PATH}

    cd gdal/
    CUSTOM_LIBS="-L${LINK_DIR}/lib -ltiff -ljpeg -lproj -lpng -lexpat"
    CUSTOM_CFLAGS="${CFLAGS} -I${LINK_DIR}/include -I${LINK_DIR}/include/libpng16"
    CUSTOM_CXXFLAGS="${CUSTOM_CFLAGS}"

    # very custom handling for libpq/postgres support
    # forcing our portable static library to be used
    MASON_LIBPQ_PATH=${LINK_DIR}/lib/libpq.a
    if [[ $(uname -s) == 'Linux' ]]; then
        # on Linux passing -Wl will lead to libtool re-positioning libpq.a in the wrong place (no longer after libgdal.a)
        # which leads to unresolved symbols
        CUSTOM_LDFLAGS="-z nodeflib ${LDFLAGS} ${MASON_LIBPQ_PATH}"
    else
        # on OSX not passing -Wl will break libtool archive creation leading to confusing arch errors
        CUSTOM_LDFLAGS="${LDFLAGS} -Wl,${MASON_LIBPQ_PATH}"
    fi
    # we have to remove -lpq otherwise it will trigger linking to system /usr/lib/libpq
    perl -i -p -e "s/\-lpq //g;" configure
    # on linux -Wl,/path/to/libpq.a still does not work for the configure test
    # so we have to force it into LIBS. But we don't do this on OS X since it breaks libtool archive logic
    if [[ $(uname -s) == 'Linux' ]]; then
        CUSTOM_LIBS="${MASON_LIBPQ_PATH} -pthread ${CUSTOM_LIBS}"
    fi

    # note: we put ${STDLIB_CXXFLAGS} into CXX instead of LDFLAGS due to libtool oddity:
    # http://stackoverflow.com/questions/16248360/autotools-libtool-link-library-with-libstdc-despite-stdlib-libc-option-pass
    if [[ $(uname -s) == 'Darwin' ]]; then
        CXX="${CXX} -stdlib=libc++ -std=c++11"
    fi

    # note: it might be tempting to build with --without-libtool
    # but I find that will only lead to a shared libgdal.so and will
    # not produce a static library even if --enable-static is passed
    LIBS="${CUSTOM_LIBS}" \
    LDFLAGS="${CUSTOM_LDFLAGS}" \
    CFLAGS="${CUSTOM_CFLAGS}" \
    CXXFLAGS="${CUSTOM_CXXFLAGS}" \
    ./configure \
        --enable-static --disable-shared \
        ${MASON_HOST_ARG} \
        --prefix=${MASON_PREFIX} \
        --with-libz=${LINK_DIR} \
        --disable-rpath \
        --with-libjson-c=internal \
        --with-geotiff=internal \
        --with-expat=${LINK_DIR} \
        --with-threads=yes \
        --with-fgdb=no \
        --with-rename-internal-libtiff-symbols=no \
        --with-rename-internal-libgeotiff-symbols=no \
        --with-hide-internal-symbols=yes \
        --with-libtiff=${LINK_DIR} \
        --with-jpeg=${LINK_DIR} \
        --with-png=${LINK_DIR} \
        --with-pg=${LINK_DIR}/bin/pg_config \
        --with-static-proj4=${LINK_DIR} \
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
        --with-grib=no \
        --with-freexl=no \
        --with-avx=no \
        --with-sse=no \
        --with-perl=no \
        --with-ruby=no \
        --with-python=no \
        --with-java=no \
        --with-podofo=no \
        --with-pam \
        --with-webp=no \
        --with-pcre=no \
        --with-liblzma=no \
        --with-netcdf=no \
        --with-poppler=no

    make -j${MASON_CONCURRENCY}
    make install

    # attempt to make paths relative in gdal-config
    python -c "data=open('$MASON_PREFIX/bin/gdal-config','r').read();open('$MASON_PREFIX/bin/gdal-config','w').write(data.replace('$MASON_PREFIX','\$( cd \"\$( dirname \$( dirname \"\$0\" ))\" && pwd )'))"
    # fix paths to all deps to point to mason_packages/.link
    python -c "data=open('$MASON_PREFIX/bin/gdal-config','r').read();open('$MASON_PREFIX/bin/gdal-config','w').write(data.replace('$MASON_ROOT','./mason_packages'))"
    # add static libpq.a
    python -c "data=open('$MASON_PREFIX/bin/gdal-config','r').read();open('$MASON_PREFIX/bin/gdal-config','w').write(data.replace('CONFIG_DEP_LIBS=\"','CONFIG_DEP_LIBS=\"-lpq'))"
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

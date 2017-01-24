#!/usr/bin/env bash

MASON_NAME=libpq
MASON_VERSION=9.4.1
MASON_LIB_FILE=lib/libpq.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/libpq.pc

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://ftp.postgresql.org/pub/source/v${MASON_VERSION}/postgresql-${MASON_VERSION}.tar.bz2 \
        1fcc75dccdb9ffd3f30e723828cbfce00c9b13fd

    mason_extract_tar_bz2

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/postgresql-${MASON_VERSION}
}

function mason_compile {
    if [[ ${MASON_PLATFORM} == 'linux' ]]; then
        mason_step "Loading patch 'https://github.com/mapbox/mason/blob/${MASON_SLUG}/patch.diff'..."
        curl --retry 3 -s -f -# -L \
          https://raw.githubusercontent.com/mapbox/mason/${MASON_SLUG}/patch.diff \
          -O || (mason_error "Could not find patch for ${MASON_SLUG}" && exit 1)
        patch src/include/pg_config_manual.h ./patch.diff
    fi

    ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --enable-thread-safety \
        --enable-largefile \
        --without-bonjour \
        --without-openssl \
        --without-pam \
        --without-krb5 \
        --without-gssapi \
        --without-ossp-uuid \
        --without-readline \
        --without-ldap \
        --without-zlib \
        --without-libxml \
        --without-libxslt \
        --without-selinux \
        --without-python \
        --without-perl \
        --without-tcl \
        --disable-rpath \
        --disable-debug \
        --disable-profiling \
        --disable-coverage \
        --disable-dtrace \
        --disable-depend \
        --disable-cassert

    make -j${MASON_CONCURRENCY} -C src/bin/pg_config install
    make -j${MASON_CONCURRENCY} -C src/interfaces/libpq/ install
    cp src/include/postgres_ext.h ${MASON_PREFIX}/include/
    cp src/include/pg_config_ext.h ${MASON_PREFIX}/include/
    rm -f ${MASON_PREFIX}/lib/libpq{*.so*,*.dylib}
}

function mason_clean {
    make clean
}

mason_run "$@"

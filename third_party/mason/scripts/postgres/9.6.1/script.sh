#!/usr/bin/env bash

MASON_NAME=postgres
MASON_VERSION=9.6.1
MASON_LIB_FILE=bin/psql

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://ftp.postgresql.org/pub/source/v${MASON_VERSION}/postgresql-${MASON_VERSION}.tar.bz2 \
        6aef3fb521aaf987a9363a314ff7d5539b6601cd

    mason_extract_tar_bz2

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/postgresql-${MASON_VERSION}
}

function mason_compile {
    if [[ ${MASON_PLATFORM} == 'linux' ]]; then
        mason_step "Loading patch"
        patch src/include/pg_config_manual.h ${MASON_DIR}/scripts/${MASON_NAME}/${MASON_VERSION}/patch.diff
    fi

    # note CFLAGS overrides defaults (-Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Wendif-labels -Wmissing-format-attribute -Wformat-security -fno-strict-aliasing -fwrapv -Wno-unused-command-line-argument) so we need to add optimization flags back
    export CFLAGS="${CFLAGS}  -O3 -DNDEBUG -Wall -Wmissing-prototypes -Wpointer-arith -Wdeclaration-after-statement -Wendif-labels -Wmissing-format-attribute -Wformat-security -fno-strict-aliasing -fwrapv -Wno-unused-command-line-argument"
    ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --enable-thread-safety \
        --enable-largefile \
        --with-python \
        --with-zlib \
        --without-bonjour \
        --without-openssl \
        --without-pam \
        --without-gssapi \
        --without-ossp-uuid \
        --without-readline \
        --without-ldap \
        --without-libxml \
        --without-libxslt \
        --without-selinux \
        --without-perl \
        --without-tcl \
        --disable-rpath \
        --disable-debug \
        --disable-profiling \
        --disable-coverage \
        --disable-dtrace \
        --disable-depend \
        --disable-cassert

    make -j${MASON_CONCURRENCY} -C src/interfaces/libpq/ install
    rm -f src/interfaces/libpq{*.so*,*.dylib}
    rm -f ${MASON_PREFIX}/lib/libpq{*.so*,*.dylib}
    MASON_LIBPQ_PATH=${MASON_PREFIX}/lib/libpq.a
    MASON_LIBPQ_PATH2=${MASON_LIBPQ_PATH////\\/}
    perl -i -p -e "s/\-lpq/${MASON_LIBPQ_PATH2} -pthread/g;" src//Makefile.global.in
    perl -i -p -e "s/\-lpq/${MASON_LIBPQ_PATH2} -pthread/g;" src//Makefile.global
    make -j${MASON_CONCURRENCY} install
    make -j${MASON_CONCURRENCY} -C contrib/hstore install
    rm -f ${MASON_PREFIX}/lib/lib{*.so*,*.dylib}
}

function mason_clean {
    make clean
}

mason_run "$@"

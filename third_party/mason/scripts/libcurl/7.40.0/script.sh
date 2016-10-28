#!/usr/bin/env bash

MASON_NAME=libcurl
MASON_VERSION=7.40.0
MASON_LIB_FILE=lib/libcurl.a
MASON_PKGCONFIG_FILE=lib/pkgconfig/libcurl.pc

. ${MASON_DIR}/mason.sh


function mason_load_source {
    mason_download \
        http://curl.haxx.se/download/curl-7.40.0.tar.gz \
        c7c97e02f5fa4302f4c25c72486359f7b46f7d6d

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/curl-${MASON_VERSION}
}

function mason_prepare_compile {
    ${MASON_DIR}/mason install openssl 1.0.1l
    MASON_OPENSSL=`${MASON_DIR}/mason prefix openssl 1.0.1l`

    if [ ${MASON_PLATFORM} = 'linux' ]; then
        LIBS="-ldl ${LIBS=}"
    fi
}

function mason_compile {
    LIBS="${LIBS=}" ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --enable-static \
        --disable-shared \
        --with-pic \
        --enable-manual \
        --with-ssl=${MASON_OPENSSL} \
        --without-ca-bundle \
        --without-ca-path \
        --without-darwinssl \
        --without-gnutls \
        --without-polarssl \
        --without-cyassl \
        --without-nss \
        --without-axtls \
        --without-libmetalink \
        --without-libssh2 \
        --without-librtmp \
        --without-winidn \
        --without-libidn \
        --without-nghttp2 \
        --disable-ldap \
        --disable-ldaps \
        --disable-ldap \
        --disable-ftp \
        --disable-file \
        --disable-rtsp \
        --disable-proxy \
        --disable-dict \
        --disable-telnet \
        --disable-tftp \
        --disable-pop3 \
        --disable-imap \
        --disable-smtp \
        --disable-gopher \
        --disable-libcurl-option \
        --disable-sspi \
        --disable-crypto-auth \
        --disable-ntlm-wb \
        --disable-tls-srp \
        --disable-cookies

    make -j${MASON_CONCURRENCY}
    make install
}

function mason_clean {
    make clean
}

mason_run "$@"

#!/usr/bin/env bash

MASON_NAME=cairo
MASON_VERSION=1.14.6
MASON_LIB_FILE=lib/libcairo.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://cairographics.org/releases/${MASON_NAME}-${MASON_VERSION}.tar.xz \
        b19d7d7b4e290eb6377ddc3688984cb66da036cb

    mason_extract_tar_xz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_prepare_compile {
    cd $(dirname ${MASON_ROOT})
    ${MASON_DIR}/mason install libpng 1.6.24
    MASON_PNG=$(${MASON_DIR}/mason prefix libpng 1.6.24)
    ${MASON_DIR}/mason install freetype 2.6.5
    MASON_FREETYPE=$(${MASON_DIR}/mason prefix freetype 2.6.5)
    ${MASON_DIR}/mason install pixman 0.34.0
    MASON_PIXMAN=$(${MASON_DIR}/mason prefix pixman 0.34.0)
}

function mason_compile {
    mason_step "Loading patch"
    patch -N -p1 < ${MASON_DIR}/scripts/${MASON_NAME}/${MASON_VERSION}/patch.diff
    CFLAGS="${CFLAGS} -Wno-enum-conversion -I${MASON_PIXMAN}/include/pixman-1 -I${MASON_FREETYPE}/include/freetype2 -I${MASON_PNG}/include/"
    LDFLAGS="-L${MASON_PIXMAN}/lib -lpixman-1 -L${MASON_FREETYPE}/lib -lfreetype -L${MASON_PNG}/lib -lpng"
    CFLAGS=${CFLAGS} LDFLAGS=${LDFLAGS} ./autogen.sh \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --enable-static --disable-shared \
        --enable-pdf=yes \
        --enable-ft=yes \
        --enable-png=yes \
        --enable-svg=yes \
        --enable-ps=yes \
        --enable-fc=no \
        --enable-script=no \
        --enable-interpreter=no \
        --enable-quartz=no \
        --enable-quartz-image=no \
        --enable-quartz-font=no \
        --enable-trace=no \
        --enable-gtk-doc=no \
        --enable-qt=no \
        --enable-win32=no \
        --enable-win32-font=no \
        --enable-skia=no \
        --enable-os2=no \
        --enable-beos=no \
        --enable-drm=no \
        --enable-gallium=no \
        --enable-gl=no \
        --enable-glesv2=no \
        --enable-directfb=no \
        --enable-vg=no \
        --enable-egl=no \
        --enable-glx=no \
        --enable-wgl=no \
        --enable-test-surfaces=no \
        --enable-tee=no \
        --enable-xml=no \
        --disable-valgrind \
        --enable-gobject=no \
        --enable-xlib=no \
        --enable-xlib-xrender=no \
        --enable-xcb=no \
        --enable-xlib-xcb=no \
        --enable-xcb-shm=no \
        --enable-full-testing=no \
        --enable-symbol-lookup=no \
        --disable-dependency-tracking
    # The -i and -k flags are to workaround make[6]: [install-data-local] Error 1 (ignored)
    make V=1 -j${MASON_CONCURRENCY} -i -k
    make install -i -k
}

function mason_clean {
    make clean
}

mason_run "$@"

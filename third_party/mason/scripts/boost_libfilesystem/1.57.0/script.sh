#!/usr/bin/env bash

BOOST_VERSION1="1.57.0"
BOOST_VERSION2="1_57_0"
BOOST_LIBRARY="filesystem"
BOOST_TOOLSET="clang"
BOOST_ARCH="x86"

MASON_NAME=boost_lib${BOOST_LIBRARY}
MASON_VERSION=1.57.0
MASON_LIB_FILE=lib/libboost_${BOOST_LIBRARY}.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://downloads.sourceforge.net/project/boost/boost/${BOOST_VERSION1}/boost_${BOOST_VERSION2}.tar.bz2 \
        397306fa6d0858c4885fbba7d43a0164dcb7f53e

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/boost_${BOOST_VERSION2}

    mason_extract_tar_bz2
}

function gen_config() {
  echo "using $1 : : $(which $2)" > user-config.jam
  if [[ "${AR:-false}" != false ]] || [[ "${RANLIB:-false}" != false ]]; then
      echo ' : ' >> user-config.jam
      if [[ "${AR:-false}" != false ]]; then
          echo "<archiver>${AR} " >> user-config.jam
      fi
      if [[ "${RANLIB:-false}" != false ]]; then
          echo "<ranlib>${RANLIB} " >> user-config.jam
      fi
  fi
  echo ' ;' >> user-config.jam
}

function mason_compile {
    mason_step "Loading patch 'https://github.com/mapbox/mason/blob/${MASON_SLUG}/patch.diff'..."
    curl --retry 3 -s -f -# -L \
      https://raw.githubusercontent.com/mapbox/mason/${MASON_SLUG}/patch.diff \
      -O || (mason_error "Could not find patch for ${MASON_SLUG}" && exit 1)
    patch -N -p0 < ./patch.diff

    gen_config ${BOOST_TOOLSET} clang++
    if [[ ! -f ./b2 ]] ; then
        ./bootstrap.sh
    fi
    CXXFLAGS="${CXXFLAGS} -fvisibility=hidden"
    ./b2 \
        --with-${BOOST_LIBRARY} \
        --prefix=${MASON_PREFIX} \
        -j${MASON_CONCURRENCY} \
        -d0 \
        --ignore-site-config --user-config=user-config.jam \
        architecture="${BOOST_ARCH}" \
        toolset="${BOOST_TOOLSET}" \
        link=static \
        variant=release \
        linkflags="${LDFLAGS:-" "}" \
        cxxflags="${CXXFLAGS:-" "}" \
        stage
    mkdir -p $(dirname ${MASON_PREFIX}/${MASON_LIB_FILE})
    mv stage/${MASON_LIB_FILE} ${MASON_PREFIX}/${MASON_LIB_FILE}
}

function mason_ldflags {
    echo "-lboost_${BOOST_LIBRARY}"
}

function mason_clean {
    make clean
}

mason_run "$@"

#!/usr/bin/env bash

BOOST_VERSION1="1.60.0"
BOOST_VERSION2="1_60_0"
BOOST_LIBRARY="program_options"
BOOST_TOOLSET="clang"
BOOST_ARCH="x86"

MASON_NAME=boost_lib${BOOST_LIBRARY}
MASON_VERSION=${BOOST_VERSION1}
MASON_LIB_FILE=lib/libboost_${BOOST_LIBRARY}.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        http://downloads.sourceforge.net/project/boost/boost/${BOOST_VERSION1}/boost_${BOOST_VERSION2}.tar.bz2 \
        40a65135d34c3e3a3cdbe681f06745c086e5b941

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
    gen_config ${BOOST_TOOLSET} clang++
    if [[ ! -f ./b2 ]] ; then
        ./bootstrap.sh
    fi
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

function mason_cflags {
    :
}

function mason_ldflags {
    :
}

function mason_clean {
    make clean
}

mason_run "$@"

#!/usr/bin/env bash

BOOST_VERSION1="1.59.0"
BOOST_VERSION2="1_59_0"
BOOST_LIBRARY="regex"
BOOST_TOOLSET="clang"
BOOST_ARCH="x86"

MASON_NAME=boost_liball_osrm
MASON_VERSION=1.59.0
# this boost package has multiple libraries to we
# reference this empty file as a placeholder for all of them
MASON_LIB_FILE=lib/libboost_placeholder.txt

. ${MASON_DIR}/mason.sh

export CXX=${CXX:-clang++}
export MASON_CONCURRENCY_OVERRIDE=${MASON_CONCURRENCY_OVERRIDE:-${MASON_CONCURRENCY}}

function mason_load_source {
    mason_download \
        http://downloads.sourceforge.net/project/boost/boost/1.59.0/boost_1_59_0.tar.bz2 \
        ff2e48f4d7e3c4b393d41e07a2f5d923b990967d

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
    if [[ -f ../../../patch.diff ]]; then
      patch -N -p0 < ../../../patch.diff
    else
      # patch to workaround crashes in python.input
      # https://github.com/mapnik/mapnik/issues/1968
      mason_step "Loading patch 'https://github.com/mapbox/mason/blob/${MASON_SLUG}/patch.diff'..."
      curl --retry 3 -s -f -# -L \
        https://raw.githubusercontent.com/mapbox/mason/${MASON_SLUG}/patch.diff \
        -O || (mason_error "Could not find patch for ${MASON_SLUG}" && exit 1)
      patch -N -p0 < ./patch.diff
    fi
    gen_config ${BOOST_TOOLSET} ${CXX}
    if [[ ! -f ./b2 ]] ; then
        ./bootstrap.sh
    fi
    CXXFLAGS="${CXXFLAGS} -fvisibility=hidden"
    ./b2 \
        --with-regex \
        --with-system \
        --with-thread \
        --with-filesystem \
        --with-program_options \
        --with-test \
        --with-date_time \
        --with-iostreams \
        --prefix=${MASON_PREFIX} \
        -j${MASON_CONCURRENCY_OVERRIDE} \
        -sHAVE_ICU=0 \
        linkflags="${LDFLAGS:-" "} ${BOOST_LDFLAGS}" \
        cxxflags="${CXXFLAGS:-" "}" \
        -d0 \
        --ignore-site-config --user-config=user-config.jam \
        architecture="${BOOST_ARCH}" \
        toolset="${BOOST_TOOLSET}" \
        link=static \
        variant=release \
        install

        mkdir -p ${MASON_PREFIX}/lib/
        touch ${MASON_PREFIX}/lib/libboost_placeholder.txt
}

function mason_ldflags {
    :
}

function mason_clean {
    make clean
}

mason_run "$@"

#!/usr/bin/env bash

function mason_load_source {
    mason_download \
        http://downloads.sourceforge.net/project/boost/boost/${MASON_VERSION}/boost_${BOOST_VERSION}.tar.bz2 \
        ${BOOST_SHASUM}

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/boost_${BOOST_VERSION}

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
    gen_config ${BOOST_TOOLSET} ${BOOST_TOOLSET_CXX}
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

function mason_prefix {
    echo "${MASON_PREFIX}"
}

function mason_cflags {
    echo "-I${MASON_PREFIX}/include"
}

function mason_ldflags {
    local LOCAL_LDFLAGS
    LOCAL_LDFLAGS="-L${MASON_PREFIX}/lib"
    if [[ ${BOOST_LIBRARY:-false} != false ]]; then
        LOCAL_LDFLAGS="${LOCAL_LDFLAGS} -l${BOOST_LIBRARY}"
    fi
    echo $LOCAL_LDFLAGS
}

#!/usr/bin/env bash

# dynamically determine the path to this package
HERE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"

# key properties unique to this library
THIS_DIR=$(basename $(dirname $HERE))
BOOST_LIBRARY=${THIS_DIR#boost_lib}
MASON_NAME=boost_lib${BOOST_LIBRARY}
MASON_LIB_FILE=lib/libboost_${BOOST_LIBRARY}.a
# hack for inconsistently named test lib
if [[ ${MASON_LIB_FILE} == "lib/libboost_test.a" ]]; then
    MASON_LIB_FILE=lib/libboost_unit_test_framework.a
fi

# inherit from boost base (used for all boost library packages)
BASE_PATH=${HERE}/../../boost/$(basename $HERE |cut -d- -f1)
source ${BASE_PATH}/base.sh

# XXX: Append the -cxx11abi prefix to the package
export BOOST_VERSION_DOWNLOAD=$MASON_VERSION
export MASON_VERSION=$MASON_VERSION-cxx11abi

# setup mason env
. ${MASON_DIR}/mason.sh

# source common build functions
source ${BASE_PATH}/common.sh

export CXXFLAGS="${CXXFLAGS} -D_GLIBCXX_USE_CXX11_ABI=1"

function mason_load_source {
    mason_download \
        http://downloads.sourceforge.net/project/boost/boost/${BOOST_VERSION_DOWNLOAD}/boost_${BOOST_VERSION}.tar.bz2 \
        ${BOOST_SHASUM}

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/boost_${BOOST_VERSION}

    mason_extract_tar_bz2
}

mason_run "$@"

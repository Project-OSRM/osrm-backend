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
BASE_PATH=${HERE}/../../boost/$(basename $HERE)
source ${BASE_PATH}/base.sh

# setup mason env
. ${MASON_DIR}/mason.sh

# source common build functions
source ${BASE_PATH}/common.sh

mason_run "$@"

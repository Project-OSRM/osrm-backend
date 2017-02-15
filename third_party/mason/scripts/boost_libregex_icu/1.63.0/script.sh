#!/usr/bin/env bash

# dynamically determine the path to this package
HERE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"

# key properties unique to this library
THIS_DIR=$(basename $(dirname $HERE))
# Note: cannot deduce from directory since it is named in a custom way
#BOOST_LIBRARY=${THIS_DIR#boost_lib}
BOOST_LIBRARY=regex
MASON_NAME=boost_lib${BOOST_LIBRARY}_icu
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

function mason_prepare_compile {
    ${MASON_DIR}/mason install icu 55.1
    MASON_ICU=$(${MASON_DIR}/mason prefix icu 55.1)
}

# custom compile that gets icu working
function mason_compile {
    gen_config ${BOOST_TOOLSET} ${BOOST_TOOLSET_CXX}
    if [[ ! -f ./b2 ]] ; then
        ./bootstrap.sh
    fi
    echo 'int main() { return 0; }' > libs/regex/build/has_icu_test.cpp
    ./b2 \
        --with-${BOOST_LIBRARY} \
        --prefix=${MASON_PREFIX} \
        -j${MASON_CONCURRENCY} \
        -sHAVE_ICU=1 -sICU_PATH=${MASON_ICU} --reconfigure --debug-configuration \
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

mason_run "$@"

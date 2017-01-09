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

function write_python_config() {
# usage:
# write_python_config <user-config.jam> <version> <base> <variant>
PYTHON_VERSION=$2
# note: apple pythons need '/System'
PYTHON_BASE=$3
# note: python 3 uses 'm'
PYTHON_VARIANT=$4
if [[ ${UNAME} == 'Darwin' ]]; then
    echo "
      using python
           : ${PYTHON_VERSION} # version
           : ${PYTHON_BASE}/Library/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/bin/python${PYTHON_VERSION}${PYTHON_VARIANT} # cmd-or-prefix
           : ${PYTHON_BASE}/Library/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/include/python${PYTHON_VERSION}${PYTHON_VARIANT} # includes
           : ${PYTHON_BASE}/Library/Frameworks/Python.framework/Versions/${PYTHON_VERSION}/lib/python${PYTHON_VERSION}/config${PYTHON_VARIANT} # a lib actually symlink
           : <toolset>${BOOST_TOOLSET} # condition
           ;
    " >> $1
else
  if [[ ${UNAME} == 'FreeBSD' ]]; then
      echo "
        using python
             : ${PYTHON_VERSION} # version
             : /usr/local/bin/python${PYTHON_VERSION}${PYTHON_VARIANT} # cmd-or-prefix
             : /usr/local/include/python${PYTHON_VERSION} # includes
             : /usr/local/lib/python${PYTHON_VERSION}/config${PYTHON_VARIANT}
             : <toolset>${BOOST_TOOLSET} # condition
             ;
      " >> $1
  else
      echo "
        using python
             : ${PYTHON_VERSION} # version
             : /usr/bin/python${PYTHON_VERSION}${PYTHON_VARIANT} # cmd-or-prefix
             : /usr/include/python${PYTHON_VERSION} # includes
             : /usr/lib/python${PYTHON_VERSION}/config${PYTHON_VARIANT}
             : <toolset>${BOOST_TOOLSET} # condition
             ;
      " >> $1
  fi
fi
}

function mason_compile {
    # patch to workaround crashes in python.input
    # https://github.com/mapnik/mapnik/issues/1968
    mason_step "Loading patch ${MASON_DIR}/scripts/${MASON_NAME}/${MASON_VERSION}/patch.diff"
    patch -N -p0 < ${MASON_DIR}/scripts/${MASON_NAME}/${MASON_VERSION}/patch.diff
    write_python_config user-config.jam "2.7" "/System" ""
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

mason_run "$@"

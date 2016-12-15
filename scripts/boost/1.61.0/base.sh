#!/usr/bin/env bash

# NOTE: use the ./util/new_boost.sh script to create new versions

export MASON_VERSION=1.61.0
export BOOST_VERSION=${MASON_VERSION//./_}
export BOOST_TOOLSET="clang"
export BOOST_TOOLSET_CXX="clang++"
export BOOST_ARCH="x86"
export BOOST_SHASUM=0a72c541e468d76a957adc14e54688dd695d566f

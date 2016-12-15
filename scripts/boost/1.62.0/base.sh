#!/usr/bin/env bash

# NOTE: use the ./utils/new_boost.sh script to create new versions

export MASON_VERSION=1.62.0
export BOOST_VERSION=${MASON_VERSION//./_}
export BOOST_TOOLSET="clang"
export BOOST_TOOLSET_CXX="clang++"
export BOOST_ARCH="x86"
export BOOST_SHASUM=f4151eec3e9394146b7bebcb17b83149de0a6c23
# special override to ensure each library shares the cached download
export MASON_DOWNLOAD_SLUG="boost-${MASON_VERSION}"

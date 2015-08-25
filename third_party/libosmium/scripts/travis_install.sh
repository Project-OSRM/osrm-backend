#!/bin/sh
#
#  travis_install.sh
#

if [ "$TRAVIS_OS_NAME" = "osx" ]; then

    brew install google-sparsehash || true

    brew install --without-python boost || true

    # workaround for gdal homebrew problem
    brew remove gdal
    brew install gdal

fi

cd ..
git clone --quiet --depth 1 https://github.com/osmcode/osm-testdata.git


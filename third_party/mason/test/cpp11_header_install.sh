#!/usr/bin/env bash

set -e -u
set -o pipefail

./mason install boost 1.57.0
./mason link boost 1.57.0

failure=0

# boost and packages other we symlink the directory 
if [[ ! -d mason_packages/.link/include/boost ]]; then
    echo "could not find expected include/boost"
    failure=1
fi

# install packages that share namespaces and directories
# and insure they get placed okay (and don't prevent each other
# from being symlinked)

./mason install sparsehash 2.0.2
./mason link sparsehash 2.0.2
./mason install protobuf 2.6.1
./mason link protobuf 2.6.1
./mason install geometry 0.7.0
./mason link geometry 0.7.0
./mason install variant 1.1.0
./mason link variant 1.1.0

if [[ ! -d mason_packages/.link/include/google/sparsehash ]]; then
    echo "could not find expected include/google/sparsehash"
    failure=1
fi


if [[ ! -d mason_packages/.link/include/google/protobuf ]]; then
    echo "could not find expected include/google/protobuf"
    failure=1
fi


if [[ ! -d mason_packages/.link/include/mapbox/geometry ]]; then
    echo "could not find expected include/mapbox/geometry"
    failure=1
fi

exit $failure



#!/usr/bin/env zsh

# # # # sol2
# The MIT License (MIT)
#
# Copyright (c) 2013-2018 Rapptz, ThePhD, and contributors
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

export SOL2_DIR=${TRAVIS_BUILD_DIR}
mkdir -p build-sol2/Debug
mkdir -p build-sol2/Release
cd build-sol2

echo "=== Compiler and tool variables ==="
ninja --version
cmake --version

cd Debug
	cmake ${SOL2_DIR} -G Xcode -DCMAKE_BUILD_TYPE=Debug -DLUA_VERSION="${LUA_VERSION}" -DCI=ON -DPLATFORM=${PLATFORM} -DBUILD_LUA=ON -DBUILD_LUA_AS_DLL=OFF -DTESTS=ON -DEXAMPLES=ON -DSINGLE=ON -DTESTS_EXAMPLES=ON -DEXAMPLES_SINGLE=ON -DTESTS_SINGLE=ON
	cmake --build . --config Debug
	ctest --build-config Debug --output-on-failure
cd ..

cd Release
	cmake ${SOL2_DIR} -G Xcode -DCMAKE_BUILD_TYPE=Release -DLUA_VERSION="${LUA_VERSION}" -DCI=ON -DPLATFORM=${PLATFORM} -DBUILD_LUA=ON -DBUILD_LUA_AS_DLL=OFF -DTESTS=ON -DEXAMPLES=ON -DSINGLE=ON -DTESTS_EXAMPLES=ON -DEXAMPLES_SINGLE=ON -DTESTS_SINGLE=ON
	cmake --build . --config Release
	ctest --build-config Release --output-on-failure
cd ..

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

# # This script runs the actual project

echo -en "travis_fold:start:build_preparation.1\r"
	if [ -z "${SOL2_DIR}" ]
	then
		if [ ${CI} = true ]
		then
			export SOL2_DIR=~/sol2
		else		
			export SOL2_DIR=../..
		fi
	fi

	if [ -z "${LUA_VERSION}" ]
	then
		export LUA_VERSION=5.3.4
	fi

	mkdir -p build-sol2
	cd build-sol2

	build_dir=$(pwd)

	if [ -f "sol2.compiler.vars" ]
	then
		source ./sol2.compiler.vars
	fi

	if [[ ${LUA_VERSION} =~ "5.3" ]]
	then
		export INTEROP_DEFINES="-DINTEROP_EXAMPLES=ON -DTESTS_INTEROP_EXAMPLES=ON -DINTEROP_EXAMPLES_SINGLE=ON -DDYNAMIC_LOADING_EXAMPLES=ON -DDYNAMIC_LOADING_EXAMPLES_SINGLE=ON -DTESTS_DYNAMIC_LOADING_EXAMPLES=ON"
	else
		export INTEROP_DEFINES=
	fi

	mkdir -p Debug Release

	export build_type_cc=-DCMAKE_C_COMPILER\=${CC}
	export build_type_cxx=-DCMAKE_CXX_COMPILER\=${CXX}
echo -en "travis_fold:end:build_preparation.1\r"


# show the tool and compiler versions we're using
echo -en "travis_fold:start:build_preparation.2\r"
	echo "=== Compiler and tool variables ==="
	ninja --version
	cmake --version
	${CC} --version
	${CXX} --version
	echo build_type_cc : "${build_type_cc}"
	echo build_type_cxx: "${build_type_cxx}"
echo -en "travis_fold:end:build_preparation.2\r"

echo -en "travis_fold:start:build.debug\r"
	cd Debug
	cmake ${SOL2_DIR} -G Ninja -DCMAKE_BUILD_TYPE=Debug   ${build_type_cc} ${build_type_cxx} -DLUA_VERSION="${LUA_VERSION}" -DCI=ON -DPLATFORM=${PLATFORM} -DBUILD_LUA=ON -DBUILD_LUA_AS_DLL=OFF -DTESTS=ON -DEXAMPLES=ON -DSINGLE=ON -DTESTS_EXAMPLES=ON -DEXAMPLES_SINGLE=ON -DTESTS_SINGLE=ON ${INTEROP_DEFINES}
	cmake --build . --config Debug
echo -en "travis_fold:end:build.debug\r"
echo -en "travis_fold:start:test.debug\r"
	ctest --build-config Debug --output-on-failure
	cd ..
echo -en "travis_fold:end:test.debug\r"

echo "travis_fold:start:build.release\r"
	cd Release
	cmake ${SOL2_DIR} -G Ninja -DCMAKE_BUILD_TYPE=Release ${build_type_cc} ${build_type_cxx} -DLUA_VERSION="${LUA_VERSION}" -DCI=ON -DPLATFORM=${PLATFORM} -DBUILD_LUA=ON -DBUILD_LUA_AS_DLL=OFF -DTESTS=ON -DEXAMPLES=ON -DSINGLE=ON -DTESTS_EXAMPLES=ON -DEXAMPLES_SINGLE=ON -DTESTS_SINGLE=ON ${INTEROP_DEFINES}
	cmake --build . --config Release
echo -en "travis_fold:end:build.release\r"
echo -en "travis_fold:start:test.release\r"
	ctest --build-config Release --output-on-failure
	cd ..
echo -en "travis_fold:end:test.release\r"

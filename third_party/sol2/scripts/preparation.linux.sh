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

top_level_dir="$(pwd)"

mkdir -p build-sol2
cd build-sol2

build_dir="$(pwd)"

echo "#\!/usr/bin/env zsh\n\n" > "sol2.compiler.vars"

# # Initial and necessary installations
apt-get update && apt-get -y install ninja-build libreadline6 libreadline6-dev lib32readline6 lib32readline6-dev python3 wget curl libcurl3 cmake git

# # LLVM and GCC updates
# Grab LLVM or GCC 
# if we need it
# defining both is probably an expotentially terrible idea
if [ "${LLVM_VERSION}" ]
then
	# get and use LLVM
	version_nums=(${=LLVM_VERSION//./ })
	major=$version_nums[1]
	minor=$version_nums[2]
	revision=$version_nums[3]
	download_llvm=true
	download_version=16.04
	apt_version=${major}.${minor}
	#sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
	#sudo apt-get -y update
	if [ ${major} -le 5 ] && [ ${major} -ge 4 ];
	then
		download_llvm=false
	elif [ ${major} -eq 3 ]
	then
		download_llvm=false
		if [ ${minor} -lt 5 ]
		then
			download_llvm=true
			download_version=14.04
		fi
	fi
	if [ ${download_llvm} = true ]
	then
		export LLVM_ARCHIVE_PATH=${build_dir}/clang+llvm.tar.xz
		export CLANG_PREFIX=${build_dir}/clang-${LLVM_VERSION}
		export PATH=$CLANG_PREFIX/bin:$PATH
		export LD_LIBRARY_PATH=$CLANG_PREFIX/lib:$LD_LIBRARY_PATH
		echo "export LLVM_ARCHIVE_PATH=${build_dir}/clang+llvm.tar.xz\nexport CLANG_PREFIX=${build_dir}/clang-$LLVM_VERSION\nexport PATH=$CLANG_PREFIX/bin:$PATH\nexport LD_LIBRARY_PATH=$CLANG_PREFIX/lib:$LD_LIBRARY_PATH\n" >> "sol2.compiler.vars"
		
		apt-get -y install xz-utils clang
		wget http://llvm.org/releases/$LLVM_VERSION/clang+llvm-${LLVM_VERSION}-x86_64-linux-gnu-ubuntu-${download_version}.tar.xz -O ${LLVM_ARCHIVE_PATH}
		mkdir -p "${CLANG_PREFIX}"
		tar xf "${LLVM_ARCHIE_PATH}" -C "${CLANG_PREFIX}" --strip-components 1
		# make sure a clang(++) of major/minor exists
		# use || true to ignore potential failures
		ln -s "clang-${major}.${minor}" "${CLANG_PREFIX}/bin/clang-${major}.${minor}" || true
		ln -s "clang-${major}.${minor}" "${CLANG_PREFIX}/bin/clang++-${major}.${minor}" || true
		rm -f "${LLVM_ARCHIVE_PATH}"
		export CC=clang-${major}.${minor}
		export CXX=clang++-${major}.${minor}
	else
		apt-get -y install clang-${apt_version}
		export CC=clang-${apt_version}
		export CXX=clang++-${apt_version}
	fi
elif [ "${GCC_VERSION}" ]
then
	# get and use GCC version that we desire
	apt-get -y install software-properties-common python-software-properties
	add-apt-repository -y ppa:ubuntu-toolchain-r/test
	apt-get -y update
	apt-get -y dist-upgrade
	apt-get -y install gcc-${GCC_VERSION} g++-${GCC_VERSION} gcc-${GCC_VERSION}-multilib g++-${GCC_VERSION}-multilib
	update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-${GCC_VERSION} 60 --slave /usr/bin/g++ g++ /usr/bin/g++-${GCC_VERSION}
	update-alternatives --config gcc
	export CC=gcc-${GCC_VERSION}
	export CXX=g++-${GCC_VERSION}
else
	apt-get -y install build-essential gcc-multilib g++-multilib
	export CC=cc
	export CXX=c++
fi

apt-get -y autoremove
apt-get -y clean
apt-get -y autoclean

# show the tool and compiler versions we're using
echo "=== Compiler and tool variables ==="
ninja --version
cmake --version
$CC --version
$CXX --version

echo "export CC=$CC\nexport CXX=$CXX\n" >> "sol2.compiler.vars"

cd "${top_level_dir}"

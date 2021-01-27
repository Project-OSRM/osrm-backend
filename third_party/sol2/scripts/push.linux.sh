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


CI=true
declare -a gcc_versions
gcc_versions=(
	4.9
	5
	6
	7
)
declare -r gcc_versions

declare -a llvm_versions
llvm_versions=(
	3.6.2
	3.7.1
	3.8.1
	3.9.1
	4.0.1
	5.0.1
)
declare -r llvm_versions

if [ -z "${DOCKER_USERNAME}" ]
then
	docker_username=
else
	docker_username=${DOCKER_USERNAME}/
fi

echo "======  =======  =======  =======  ======"
echo "======  Pushing All Docker Images  ======"
echo "======  =======  =======  =======  ======"

for i in $gcc_versions; do
	GCC_VERSION=$i
	unset LLVM_VERSION
	echo "====== Pushing Docker Image: ${docker_username}sol2:gcc-${GCC_VERSION}_llvm-${LLVM_VERSION} ======="
	docker push ${docker_username}sol2:gcc-${GCC_VERSION}_llvm-${LLVM_VERSION}
done

for i in $llvm_versions; do
	LLVM_VERSION=$i
	unset GCC_VERSION
	echo "====== Pushing Docker Image: ${docker_username}sol2:gcc-${GCC_VERSION}_llvm-${LLVM_VERSION} ======="
	docker push ${docker_username}sol2:gcc-${GCC_VERSION}_llvm-${LLVM_VERSION}
done

unset LLVM_VERSION
unset GCC_VERSION
echo "====== Pushing Docker Image: ${docker_username}sol2:gcc-${GCC_VERSION}_llvm-${LLVM_VERSION} ======="
docker push ${docker_username}sol2:gcc-${GCC_VERSION}_llvm-${LLVM_VERSION}


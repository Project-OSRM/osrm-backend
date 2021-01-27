# # # # sol2
# The MIT License (MIT)
# 
# Copyright (c) 2013-2017 Rapptz, ThePhD, and contributors
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

# Start from the ubuntu:xenial image
FROM ubuntu:xenial
# owner
LABEL author="ThePhD <phdofthehouse@gmail.com>"
LABEL maintainer="ThePhD <phdofthehouse@gmail.com>"
# We want our working directory to be the home directory
WORKDIR /root

# RUN is how you write to the image you've pulled down
# RUN actions are "committed" to the image, and everything will
# start from the base after all run commands are executed
RUN apt-get update && apt-get install -y \
    zsh

# Scripts should be added directly to the docker image to get us started
# We can mount the whole sol2 directory later as a volume
ADD scripts/ /root/sol2-scripts

RUN mkdir -p /root/build-sol2/Debug /root/build-sol2/Release
RUN chmod +x /root/sol2-scripts/preparation.linux.sh

VOLUME /root/sol2
#ADD . /root/sol2

# # Above this is more or less static parts: the rest is non-static
# # This is ordered like this so making multiple of these
# # containers is more or less identical up to this point
# Command line arguments, with default values
ARG CI=true
ARG GCC_VERSION
ARG LLVM_VERSION
ARG PLATFORM=x64

# Potential environment variables
ENV CI=${CI} PLATFORM=${PLATFORM} GCC_VERSION=${GCC_VERSION} LLVM_VERSION=${LLVM_VERSION} SOL2_DIR=/root/sol2

RUN ["/usr/bin/env", "zsh", "-e", "/root/sol2-scripts/preparation.linux.sh"]

# CMD/ENTRYPOINT is different from RUN
# these are done on a per-instantiation and essentially describe
# the DEFAULT behavior of this container when its started, not what state it
# gets "saved" in...
# it only runs the last CMD/ENTRYPOINT as the default behavior:
# multiple CMDs will not be respected
ENTRYPOINT ["/usr/bin/env", "zsh", "-e", "/root/sol2/scripts/run.linux.sh"]

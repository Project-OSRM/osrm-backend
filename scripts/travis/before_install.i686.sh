#!/bin/sh -ex

## Generate code for 32-bit ABI with default for x86_84 fpmath
export CFLAGS='-m32 -msse2 -mfpmath=sse'
export CXXFLAGS='-m32 -msse2 -mfpmath=sse'

sudo dpkg --add-architecture i386
sudo add-apt-repository --yes ppa:ubuntu-toolchain-r/test && ( sudo apt-get update -qq --yes || true )

sudo apt-get install -qq --yes --force-yes g++-6-multilib libxml2-dev:i386 libexpat1-dev:i386 libzip-dev:i386 libbz2-dev:i386 libstxxl-dev:i386 libtbb-dev:i386 lua5.2:i386 liblua5.2-dev:i386 libluabind-dev:i386 libboost-date-time-dev:i386 libboost-filesystem-dev:i386 libboost-iostreams-dev:i386 libboost-program-options-dev:i386 libboost-regex-dev:i386 libboost-system-dev:i386 libboost-thread-dev:i386 libboost-test-dev:i386

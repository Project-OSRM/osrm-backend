#!/bin/sh -ex

# workaround for gcc4.8 https://gcc.gnu.org/bugzilla/show_bug.cgi?id=55642
export CXXFLAGS=-Wa,-mimplicit-it=thumb

UBUNTU_RELEASE=$(lsb_release -sc)

sudo dpkg --add-architecture armhf

sudo tee -a /etc/apt/sources.list > /dev/null <<EOF
deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports ${UBUNTU_RELEASE} restricted main multiverse universe
deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports ${UBUNTU_RELEASE}-security restricted main multiverse universe
deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports ${UBUNTU_RELEASE}-updates restricted main multiverse universe
EOF
cat /etc/apt/sources.list
sudo apt-get update -qq --yes || true


sudo apt-get install -qq --yes --force-yes g++-4.8-arm-linux-gnueabihf g++-4.8-multilib-arm-linux-gnueabihf gcc-4.8-arm-linux-gnueabihf gcc-4.8-multilib-arm-linux-gnueabihf
sudo apt-get install -qq --yes --force-yes libexpat1-dev:armhf zlib1g-dev:armhf libbz2-dev:armhf libboost-date-time-dev:armhf libboost-filesystem-dev:armhf libboost-iostreams-dev:armhf libboost-program-options-dev:armhf libboost-regex-dev:armhf libboost-system-dev:armhf libboost-thread-dev:armhf libtbb-dev:armhf libboost-test-dev:armhf

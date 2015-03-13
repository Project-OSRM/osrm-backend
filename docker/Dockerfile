FROM ubuntu:14.04

RUN apt-get update -y && \
apt-get install -y build-essential git-core python-pip python-software-properties software-properties-common && \
rm -rf /var/lib/apt/lists/*

RUN apt-get -y install gcc-4.8 g++-4.8 curl automake gdb libtool make cmake pkg-config libboost1.55-dev llvm-3.4 \
protobuf-compiler libprotoc-dev libprotobuf7 libprotobuf-dev libbz2-dev libstxxl-dev libstxxl1 libxml2-dev \
libzip-dev lua5.1 liblua5.1-0-dev rubygems libtbb-dev libgdal-dev && \
pip install awscli

# luabind
RUN curl https://gist.githubusercontent.com/DennisOSRM/f2eb7b948e6fe1ae319e/raw/install-luabind.sh | sudo bash
# osmosis
RUN curl -s https://gist.githubusercontent.com/DennisOSRM/803a64a9178ec375069f/raw/ | sudo bash
# osmpbf library
RUN curl -s https://gist.githubusercontent.com/DennisOSRM/13b1b4fe38a57ead850e/raw/install_osmpbf.sh | sudo bash

RUN useradd -ms /bin/bash mapbox
USER mapbox
ENV HOME /home/mapbox
WORKDIR /home/mapbox

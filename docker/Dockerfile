FROM ubuntu:14.04

RUN apt-get update -y
RUN apt-get install -y build-essential git-core python-pip python-software-properties software-properties-common

RUN apt-get -y install gcc-4.8 g++-4.8 libboost1.55-all-dev llvm-3.4
RUN apt-get -y install libbz2-dev libstxxl-dev libstxxl1 libxml2-dev
RUN apt-get -y install libzip-dev lua5.1 liblua5.1-0-dev libtbb-dev libgdal-dev ruby1.9
RUN apt-get -y install curl cmake cmake-curses-gui

RUN pip install awscli

# luabind
RUN curl https://gist.githubusercontent.com/DennisOSRM/f2eb7b948e6fe1ae319e/raw/install-luabind.sh | sudo bash

RUN useradd -ms /bin/bash mapbox
USER mapbox
ENV HOME /home/mapbox
WORKDIR /home/mapbox

FROM ubuntu:14.04

RUN apt-get update -y
RUN apt-get install -y build-essential git-core python-pip python-software-properties software-properties-common

RUN apt-get -y install gcc-4.8 g++-4.8 libboost1.55-all-dev llvm-3.4
RUN apt-get -y install libbz2-dev libstxxl-dev libstxxl1 libxml2-dev
RUN apt-get -y install libzip-dev lua5.1 liblua5.1-0-dev libtbb-dev libgdal-dev
RUN apt-get -y install curl cmake cmake-curses-gui

RUN pip install awscli


# luabind
RUN curl https://gist.githubusercontent.com/DennisOSRM/f2eb7b948e6fe1ae319e/raw/install-luabind.sh | sudo bash

WORKDIR /opt
RUN git clone --depth 1 --branch v0.31.0 https://github.com/creationix/nvm.git
RUN /bin/bash -c "source /opt/nvm/nvm.sh && nvm install v4"

RUN useradd -ms /bin/bash mapbox
USER mapbox
ENV HOME /home/mapbox
WORKDIR /home/mapbox

RUN echo "source /opt/nvm/nvm.sh" > .bashrc
RUN echo "source /home/mapbox/.bashrc" > .profile

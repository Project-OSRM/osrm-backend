FROM ubuntu:14.04

RUN apt-get update -y && apt-get install -y  software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt-get update -y && apt-get install -y g++-5 libbz2-dev libstxxl-dev libstxxl1 libxml2-dev libzip-dev lua5.1 liblua5.1-0-dev libtbb-dev libgdal-dev libboost-all-dev ccache
RUN apt-get -y install curl cmake cmake-curses-gui git

WORKDIR /opt
RUN git clone --depth 1 --branch v0.31.0 https://github.com/creationix/nvm.git
RUN /bin/bash -c "source /opt/nvm/nvm.sh && nvm install v4"

RUN useradd -ms /bin/bash mapbox
USER mapbox
ENV HOME /home/mapbox
WORKDIR /home/mapbox

RUN echo "source /opt/nvm/nvm.sh" > .bashrc
RUN echo "source /home/mapbox/.bashrc" > .profile

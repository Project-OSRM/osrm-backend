FROM ubuntu

MAINTAINER Robin Roettger

RUN apt-get update -y && apt-get install -y  software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt-get update -y && apt-get install -y g++-5 libbz2-dev libstxxl-dev libstxxl1v5 libxml2-dev libzip-dev lua5.1 liblua5.1-0-dev libtbb-dev libgdal-dev libluabind-dev libboost-all-dev ccache
RUN apt-get -y install curl cmake cmake-curses-gui git


RUN mkdir -p /osrm-src \
 && mkdir -p /osrm-build \
 && mkdir -p /osrm-data

WORKDIR /osrm-build

RUN git clone https://github.com/beemogmbh/osrm-backend.git /osrm-src \
 && cd /osrm-src \
 && git fetch \
 && git checkout master \
 && cd /osrm-build \
 && cmake /osrm-src -DCMAKE_BUILD_TYPE=Release\
 && cmake --build . \
 && mv /osrm-src/profiles/car.lua profile.lua \
 && mv /osrm-src/profiles/lib/ lib \
 && echo "disk=/tmp/stxxl,25000,syscall" > .stxxl \
 && rm -rf /osrm-src


# Cleanup --------------------------------

RUN apt-get clean \
 && rm -rf /var/lib/apt/lists/*

# Publish --------------------------------

COPY docker-entrypoint.sh /
RUN chmod +x /docker-entrypoint.sh
ENTRYPOINT ["/docker-entrypoint.sh"]


EXPOSE 5000



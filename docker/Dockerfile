FROM ubuntu:16.04

WORKDIR /opt

RUN NPROC=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || 1) && \
    apt-get update -y && \
    apt-get install -y \
        build-essential \
        git \
        cmake \
        pkg-config \
        libbz2-dev \
        libstxxl-dev \
        libstxxl1v5 \
        libxml2-dev \
        libzip-dev \
        libboost-all-dev \
        lua5.2 \
        liblua5.2-dev \
        libtbb-dev \
        libjemalloc-dev

ARG DOCKER_TAG

RUN mkdir /src
COPY . /src
WORKDIR /src

RUN NPROC=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || 1) && \
    echo "Building OSRM ${DOCKER_TAG}" && \
    git show --format="%H" | head -n1 > /opt/OSRM_GITSHA && \
    echo "Building OSRM gitsha $(cat /opt/OSRM_GITSHA)" && \
    export LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libjemalloc.so.1 && \
    mkdir build && \
    cd build && \
    BUILD_TYPE="Release" && \
    ENABLE_ASSERTIONS="Off" && \
    BUILD_TOOLS="Off" && \
    case ${DOCKER_TAG} in *"-debug"*) BUILD_TYPE="Debug";; esac && \
    case ${DOCKER_TAG} in *"-assertions"*) BUILD_TYPE="RelWithDebInfo" && ENABLE_ASSERTIONS="On" && BUILD_TOOLS="On";; esac && \
    echo "Building ${BUILD_TYPE} with ENABLE_ASSERTIONS=${ENABLE_ASSERTIONS} BUILD_TOOLS=${BUILD_TOOLS}" && \
    cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DENABLE_ASSERTIONS=${ENABLE_ASSERTIONS} -DBUILD_TOOLS=${BUILD_TOOLS} -DENABLE_LTO=On && \
    make -j${NPROC} install && \
    cd ../profiles && \
    cp -r * /opt && \
    cd /opt && \
    \
    echo "Cleaning up" && \
    strip /usr/local/bin/* && \
    rm -rf /src && \
    apt-get autoremove -y

WORKDIR /opt

EXPOSE 5000

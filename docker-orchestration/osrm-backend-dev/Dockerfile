
FROM debian:stretch-slim

# Install necessary packages for proper system state
RUN DEBIAN_FRONTEND=noninteractive apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
  build-essential \
  cmake \
  curl \
  wget \
  tmux \
  vim \
  git \
  libboost-all-dev \
  libbz2-dev \
  libtbb-dev \
  libxml2-dev \
  libzip-dev \
  lua5.2 \
  liblua5.2-dev \
  libluabind-dev \
  pkg-config

# install go
RUN wget --progress=dot:mega https://dl.google.com/go/go1.12.5.linux-amd64.tar.gz && \
  tar -zxf go1.12.5.linux-amd64.tar.gz && \
  mv go /usr/local/ && \
  rm -f go1.12.5.linux-amd64.tar.gz && \
  mkdir -p /workspace/go/bin /workspace/go/src /workspace/go/pkg

# set env
ENV GO_HOME=/usr/local/go
ENV PATH=${GO_HOME}/bin/:$PATH
ENV GOPATH=/workspace/go



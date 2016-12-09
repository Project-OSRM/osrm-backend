#!/usr/bin/env bash

NODE_HOME=$HOME/node
export NODE_HOME
mkdir ${NODE_HOME}
if [ "${TRAVIS_OS_NAME}" == "osx" ]; then
  curl https://s3.amazonaws.com/mapbox/apps/install-node/v2.0.0/run | NV=4.4.2 NP=darwin-x64 OD=${NODE_HOME} sh
else
  curl https://s3.amazonaws.com/mapbox/apps/install-node/v2.0.0/run | NV=4.4.2 NP=linux-x64 OD=${NODE_HOME} sh
fi

PATH="${NODE_HOME}/bin:$PATH"
export PATH
node --version
npm --version
which node



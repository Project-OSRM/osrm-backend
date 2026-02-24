#!/usr/bin/env bash

# call this from the project root directory
# use once after a git clone
# usage: scripts/post_checkout.sh

# install node dependencies
npm ci --ignore-scripts

# install python packages
scripts/install_venv.sh

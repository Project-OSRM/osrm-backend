#!/usr/bin/env bash

# call this from the project root directory
# use once after a git clone
# usage: scripts/install_venv.sh

pip install -r requirements*.txt
python -m venv .venv
source scripts/activate_venv
conan profile detect --force

#!/usr/bin/env bash

set -e -u
set -o pipefail

export MASON_PLATFORM=android
export MASON_ANDROID_ARCH=arm
./mason build freetype 2.5.5
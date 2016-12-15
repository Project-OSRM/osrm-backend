#!/usr/bin/env bash

set -eu
set -o pipefail

$(dirname $0)/unit.sh
$(dirname $0)/c_install.sh
$(dirname $0)/c_build.sh
$(dirname $0)/c_build_android.sh
$(dirname $0)/c_install_symlink_includes.sh
$(dirname $0)/cpp11_header_install.sh
$(dirname $0)/cpp11_install.sh
$(dirname $0)/cpp11_build.sh
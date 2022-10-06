#!/usr/bin/env bash

set -o errexit
set -o pipefail
set -o nounset

# Note: once the subtree merge from this script has been committed and pushed to
# a branch do not attempt to rebase the branch back onto master or the subdirectory
# structure will be lost.
# http://git.661346.n2.nabble.com/subtree-merges-lose-prefix-after-rebase-td7332850.html

OSMIUM_PATH="osmcode/libosmium"
OSMIUM_TAG=v2.14.0

VARIANT_PATH="mapbox/variant"
VARIANT_TAG=v1.1.3

SOL_PATH="ThePhD/sol2"
SOL_TAG=v2.17.5

RAPIDJSON_PATH="Tencent/rapidjson"
RAPIDJSON_TAG=v1.1.0

MICROTAR_PATH="rxi/microtar"
MICROTAR_TAG=v0.1.0

PROTOZERO_PATH="mapbox/protozero"
PROTOZERO_TAG=v1.6.2

VTZERO_PATH="mapbox/vtzero"
VTZERO_TAG=v1.0.1

function update_subtree () {
    name=${1^^}
    path=$(tmpvar=${name}_PATH && echo ${!tmpvar})
    tag=$(tmpvar=${name}_TAG && echo ${!tmpvar})
    dir=$(basename $path)
    repo="https://github.com/${path}.git"
    latest=$(curl -s "https://api.github.com/repos/${path}/releases/latest" | jq ".tag_name")

    echo "Latest $1 release is ${latest}, pulling in \"${tag}\""

    read -p "Update ${1} (y/n) " ok

    if [[ $ok =~ [yY] ]]
    then
        if [ -d "third_party/$dir" ]; then
            git subtree pull -P third_party/$dir ${repo} ${tag} --squash
        else
            git subtree add -P third_party/$dir ${repo} ${tag} --squash
        fi
    fi
}

## Update dependencies
for dep in osmium variant sol rapidjson microtar protozero vtzero ; do
    update_subtree $dep
done

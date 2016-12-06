#!/usr/bin/env bash

set -o errexit
set -o pipefail
set -o nounset

# Note: once the subtree merge from this script has been committed and pushed to
# a branch do not attempt to rebase the branch back onto master or the subdirectory
# structure will be lost.
# http://git.661346.n2.nabble.com/subtree-merges-lose-prefix-after-rebase-td7332850.html

OSMIUM_REPO=https://github.com/osmcode/libosmium.git
OSMIUM_TAG=v2.10.2

VARIANT_REPO=https://github.com/mapbox/variant.git
VARIANT_TAG=v1.1.0

MASON_REPO=https://github.com/mapbox/mason.git
MASON_TAG=v0.1.1

SOL_REPO="https://github.com/ThePhD/sol2.git"
SOL_TAG=v2.15.4

VARIANT_LATEST=$(curl https://api.github.com/repos/mapbox/variant/releases/latest | jq ".tag_name")
OSMIUM_LATEST=$(curl https://api.github.com/repos/osmcode/libosmium/releases/latest | jq ".tag_name")
MASON_LATEST=$(curl https://api.github.com/repos/mapbox/mason/releases/latest | jq ".tag_name")
SOL_LATEST=$(curl "https://api.github.com/repos/ThePhD/sol2/releases/latest" | jq ".tag_name")

echo "Latest osmium release is $OSMIUM_LATEST, pulling in \"$OSMIUM_TAG\""
echo "Latest variant release is $VARIANT_LATEST, pulling in \"$VARIANT_TAG\""
echo "Latest mason release is $MASON_LATEST, pulling in \"$MASON_TAG\""
echo "Latest sol2 release is $SOL_LATEST, pulling in \"$SOL_TAG\""

read -p "Looks good? (Y/n) " ok

if [[ $ok =~ [yY] ]]
then
  git subtree pull -P third_party/libosmium/ $OSMIUM_REPO $OSMIUM_TAG --squash
  git subtree pull -P third_party/variant/ $VARIANT_REPO $VARIANT_TAG --squash
  git subtree pull -P third_party/mason/ $MASON_REPO $MASON_TAG --squash
  git subtree pull -P third_party/sol2/sol2/ $SOL_REPO $SOL_TAG --squash
fi

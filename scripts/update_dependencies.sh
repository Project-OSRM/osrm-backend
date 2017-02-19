#!/usr/bin/env bash

set -o errexit
set -o pipefail
set -o nounset

# Note: once the subtree merge from this script has been committed and pushed to
# a branch do not attempt to rebase the branch back onto master or the subdirectory
# structure will be lost.
# http://git.661346.n2.nabble.com/subtree-merges-lose-prefix-after-rebase-td7332850.html

OSMIUM_REPO="https://github.com/osmcode/libosmium.git"
OSMIUM_TAG=v2.11.0

VARIANT_REPO="https://github.com/mapbox/variant.git"
VARIANT_TAG=v1.1.0

SOL_REPO="https://github.com/ThePhD/sol2.git"
SOL_TAG=v2.15.8

VARIANT_LATEST=$(curl "https://api.github.com/repos/mapbox/variant/releases/latest" | jq ".tag_name")
OSMIUM_LATEST=$(curl "https://api.github.com/repos/osmcode/libosmium/releases/latest" | jq ".tag_name")
SOL_LATEST=$(curl "https://api.github.com/repos/ThePhD/sol2/releases/latest" | jq ".tag_name")

echo "Latest osmium release is $OSMIUM_LATEST, pulling in \"$OSMIUM_TAG\""
echo "Latest variant release is $VARIANT_LATEST, pulling in \"$VARIANT_TAG\""
echo "Latest sol2 release is $SOL_LATEST, pulling in \"$SOL_TAG\""

read -p "Update osmium (y/n) " ok
if [[ $ok =~ [yY] ]]
then
  if [ -d "third_party/libosmium" ]; then
    git subtree pull -P third_party/libosmium/ $OSMIUM_REPO $OSMIUM_TAG --squash
  else
    git subtree add -P third_party/libosmium/ $OSMIUM_REPO $OSMIUM_TAG --squash
  fi
fi

read -p "Update variant (y/n) " ok
if [[ $ok =~ [yY] ]]
then
  if [ -d "third_party/variant" ]; then
    git subtree pull -P third_party/variant/ $VARIANT_REPO $VARIANT_TAG --squash
  else
    git subtree add -P third_party/variant/ $VARIANT_REPO $VARIANT_TAG --squash
  fi
fi

read -p "Update sol2 (y/n) " ok
if [[ $ok =~ [yY] ]]
then
  if [ -d "third_party/sol2" ]; then
    git subtree pull -P third_party/sol2/sol2/ $SOL_REPO $SOL_TAG --squash
  else
    git subtree add -P third_party/sol2/sol2/ $SOL_REPO $SOL_TAG --squash
  fi
fi



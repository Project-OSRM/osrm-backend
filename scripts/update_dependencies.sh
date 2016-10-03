#!/usr/bin/env bash

OSMIUM_REPO=https://github.com/osmcode/libosmium.git
OSMIUM_TAG=v2.9.0

VARIANT_REPO=https://github.com/mapbox/variant.git
VARIANT_TAG=v1.1.0

VARIANT_LATEST=$(curl https://api.github.com/repos/mapbox/variant/releases/latest | jq ".tag_name")
OSMIUM_LATEST=$(curl https://api.github.com/repos/osmcode/libosmium/releases/latest | jq ".tag_name")

echo "Latest osmium release is $OSMIUM_LATEST, pulling in \"$OSMIUM_TAG\""
echo "Latest variant release is $VARIANT_LATEST, pulling in \"$VARIANT_TAG\""

read -p "Looks good? (Y/n) " ok

if [[ $ok =~ [yY] ]]
then
  git subtree pull -P third_party/libosmium/ $OSMIUM_REPO $OSMIUM_TAG --squash
  git subtree pull -P third_party/variant/ $VARIANT_REPO $VARIANT_TAG --squash
fi

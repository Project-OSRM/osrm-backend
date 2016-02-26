#!/usr/bin/env bash

OSMIUM_REPO=https://github.com/osmcode/libosmium.git
OSMIUM_TAG=v2.6.0
OSMIUM_LATEST=$(curl https://api.github.com/repos/osmcode/libosmium/releases/latest | jq ".tag_name")

echo "Latest osmium release is $OSMIUM_LATEST, pulling in \"$OSMIUM_TAG\""

read -p "Looks good? (Y/n) " ok

if [[ $ok =~ [yY] ]]
then
  git subtree pull -P third_party/libosmium/ $OSMIUM_REPO $OSMIUM_TAG --squash
fi

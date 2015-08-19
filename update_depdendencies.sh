#!/usr/bin/env bash

OSMIUM_REPO=https://github.com/osmcode/libosmium.git
OSMIUM_TAG=v2.3.0

VARIANT_REPO=https://github.com/mapbox/variant.git 
VARIANT_TAG=v1.0


git subtree pull -P third_party/libosmium/ $OSMIUM_REPO $OSMIUM_TAG --squash
git subtree pull -P third_party/variant/ $VARIANT_REPO $VARIANT_TAG --squash

#!/bin/sh

sudo add-apt-repository ppa:jonathonf/binutils --yes || true
sudo apt-get update -qq --yes || true
sudo apt-get install -qq --yes --force-yes binutils

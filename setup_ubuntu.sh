#!/bin/sh
set -e

sudo apt-get update
sudo apt-get install nodejs npm nodejs-legacy
sudo npm install -g grunt-cli
npm install --build-from-source

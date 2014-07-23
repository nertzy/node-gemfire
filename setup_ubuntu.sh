#!/bin/sh
set -e

sudo apt-get update
sudo apt-get install nodejs npm nodejs-legacy
sudo npm install -g grunt-cli
sudo update-alternatives --install /usr/bin/node node /usr/bin/nodejs 10
npm install --build-from-source

#!/bin/sh

set -e

cd /project && npm install --build-from-source && grunt


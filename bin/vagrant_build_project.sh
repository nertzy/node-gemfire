#!/bin/sh

set -e

cd /project
npm install
bundle install
grunt


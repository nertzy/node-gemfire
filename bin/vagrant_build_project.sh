#!/bin/sh

set -e

cd /project
npm install
gem install bundler
rbenv rehash
bundle install
grunt


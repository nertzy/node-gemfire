#!/bin/sh

set -e

cd /vagrant
npm install
gem install bundler
rbenv rehash
bundle install
bin/ci

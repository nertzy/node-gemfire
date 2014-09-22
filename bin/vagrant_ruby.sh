#!/bin/sh

set -e

if [ ! -e ~/.rbenv ]; then
  git clone https://github.com/sstephenson/rbenv.git ~/.rbenv
  echo 'export PATH="$HOME/.rbenv/bin:$PATH"' >> ~/.bash_profile
  echo 'eval "$(rbenv init -)"' >> ~/.bash_profile
fi

if [ ! -e ~/.rbenv/plugins/ruby-build ]; then
  git clone https://github.com/sstephenson/ruby-build.git ~/.rbenv/plugins/ruby-build
fi

export PATH="~/.rbenv/bin:$PATH"
eval "$(rbenv init -)"
rbenv install 2.1.3 --skip-existing
rbenv global 2.1.3
gem install bundler

#!/bin/bash

set -e -x

echo "Setting up Centos 6.5"

if ! yum -C repolist | grep epel ; then
  sudo rpm --import https://fedoraproject.org/static/0608B895.txt
  sudo rpm -Uvh http://download-i2.fedoraproject.org/pub/epel/6/i386/epel-release-6-8.noarch.rpm
fi

sudo yum -y install nodejs npm --enablerepo=epel

test -e /usr/bin/grunt || npm install -g grunt-cli

sudo sh -c "cat > /etc/profile.d/gfcpp.sh" <<'EOF'
export GFCPP=/project/vendor/NativeClient_Linux_64bit_7020_b6036
EOF


#!/bin/sh

# silences future warnings about tty
sed -i 's/^mesg n$/tty -s \&\& mesg n/g' /root/.profile

set -e
export DEBIAN_FRONTEND=noninteractive

test -e /etc/apt/sources.list.d/webupd8team-java-trusty.list || sudo add-apt-repository ppa:webupd8team/java
sudo apt-get -q -y update
echo "=== Packages Updated ==="

# Silence the Oracle Java JDK License EULA questions for automatic installation
echo debconf shared/accepted-oracle-license-v1-1 select true | \
  sudo debconf-set-selections
echo debconf shared/accepted-oracle-license-v1-1 seen true | \
  sudo debconf-set-selections
test -e /usr/bin/java || sudo apt-get -q -y install oracle-java7-installer oracle-java7-set-default
INSTALLED_JAVA_VERSION=`java -version 2>&1 | head -n1`
echo "=== Java Installed (${INSTALLED_JAVA_VERSION}) ==="

sudo apt-get -q -y install nodejs npm nodejs-legacy git unzip
echo "=== Installed node.js ==="

test -e /usr/local/bin/grunt || sudo npm install -g grunt-cli
echo "=== Installed node grunt ==="

echo "=== Host setup complete ==="

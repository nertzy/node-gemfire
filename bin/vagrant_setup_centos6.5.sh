#!/bin/bash
JAVA_RPM_URL="http://download.oracle.com/otn-pub/java/jdk/7u65-b17/jdk-7u65-linux-x64.rpm"
GEMFIRE_SERVER_FILENAME="pivotal-gemfire-8.0.0-48398.el6.noarch.rpm"
NATIVE_CLIENT_FILENAME="Pivotal_GemFire_NativeClient_Linux_64bit_8000_b6169.zip"

set -e

echo "Setting up Centos 6.5"

if [ ! -e /usr/bin/gemfire ]; then
  if [ ! -e /project/tmp/$GEMFIRE_SERVER_FILENAME ]; then
    1>&2 echo "Please download $GEMFIRE_SERVER_FILENAME from https://network.pivotal.io/products/pivotal-gemfire and place in the tmp/ subdirectory of this project"
    exit 1
  fi
  sudo -E rpm -ivh /project/tmp/$GEMFIRE_SERVER_FILENAME
fi

if [ ! -e /opt/pivotal/NativeClient_Linux_64bit_8000_b6169 ]; then
  if [ ! -e /project/tmp/$NATIVE_CLIENT_FILENAME ]; then
    1>&2 echo "Please download $NATIVE_CLIENT_FILENAME from https://network.pivotal.io/products/pivotal-gemfire and place in the tmp/ subdirectory of this project"
    exit 1
  fi
  cd /opt/pivotal
  unzip /project/tmp/$NATIVE_CLIENT_FILENAME
fi

if ! yum -C repolist | grep epel ; then
  sudo rpm --import https://fedoraproject.org/static/0608B895.txt
  sudo rpm -Uvh http://download-i2.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm
fi

sudo yum -y install nodejs --enablerepo=epel
sudo yum -y install gcc-c++ gdb git gtest gtest-devel unzip valgrind yum-utils yum-plugin-auto-update-debug-info.noarch
sudo debuginfo-install -y nodejs

if [ ! -e /usr/bin/npm ]; then
  curl -L https://npmjs.org/install.sh | skipclean=1 sh
fi

test -e /usr/bin/grunt || npm install -g grunt-cli

if [ ! -e /usr/bin/javac ]; then
  rm -f /tmp/java.rpm
  wget --no-verbose -O /tmp/java.rpm --no-check-certificate --no-cookies --header "Cookie: oraclelicense=accept-securebackup-cookie" $JAVA_RPM_URL
  sudo -E rpm -ivh /tmp/java.rpm
fi

sudo sh -c "cat > /etc/profile.d/gfcpp.sh" <<'EOF'
export GFCPP=/opt/pivotal/NativeClient_Linux_64bit_8000_b6169
export GEMFIRE=/opt/pivotal/gemfire/Pivotal_GemFire_800
export JAVA_HOME=/usr/java/default
export PATH=$GFCPP/bin:$PATH
export LD_LIBRARY_PATH=$GFCPP/lib:$LD_LIBRARY_PATH
EOF

sudo wget https://google-styleguide.googlecode.com/svn/trunk/cpplint/cpplint.py -O /usr/local/bin/cpplint.py
sudo chmod +x /usr/local/bin/cpplint.py

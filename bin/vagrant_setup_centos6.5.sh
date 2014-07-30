#!/bin/bash
JAVA_RPM_URL="http://download.oracle.com/otn-pub/java/jdk/7u65-b17/jdk-7u65-linux-x64.rpm"
GEMFIRE_RPM_URL="http://download.pivotal.com.s3.amazonaws.com/gemfire/7.0.2/pivotal-gemfire-7.0.2-1.el6.noarch.rpm"

set -e -x

echo "Setting up Centos 6.5"

if ! yum -C repolist | grep epel ; then
  sudo rpm --import https://fedoraproject.org/static/0608B895.txt
  sudo rpm -Uvh http://download-i2.fedoraproject.org/pub/epel/6/i386/epel-release-6-8.noarch.rpm
fi

sudo yum -y install nodejs npm --enablerepo=epel

test -e /usr/bin/grunt || npm install -g grunt-cli

if [ ! -e /usr/bin/javac ]; then
  rm -f /tmp/java.rpm
  wget --no-verbose -O /tmp/java.rpm --no-check-certificate --no-cookies --header "Cookie: oraclelicense=accept-securebackup-cookie" $JAVA_RPM_URL
  sudo -E rpm -ivh /tmp/java.rpm
fi

if [ ! -e /usr/bin/gemfire ]; then
  rm -f /tmp/gemfire.rpm
  wget --no-verbose -O /tmp/gemfire.rpm $GEMFIRE_RPM_URL
  sudo -E rpm -ivh /tmp/gemfire.rpm
fi

sudo sh -c "cat > /etc/profile.d/gfcpp.sh" <<'EOF'
export GFCPP=/project/vendor/NativeClient_Linux_64bit_7020_b6036
export GEMFIRE=/opt/pivotal/gemfire/Pivotal_GemFire_702
export JAVA_HOME=/usr/java/default
export PATH=$GFCPP/bin:$PATH
export LD_LIBRARY_PATH=$GFCPP/lib:$LD_LIBRARY_PATH
EOF


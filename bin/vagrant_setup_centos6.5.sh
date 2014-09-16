#!/bin/bash
S3_PREFIX="http://download.pivotal.com.s3.amazonaws.com/gemfire/8.0.0/"
GEMFIRE_SERVER_FILENAME="pivotal-gemfire-8.0.0-48398.el6.noarch.rpm"
NATIVE_CLIENT_FILENAME="Pivotal_GemFire_NativeClient_Linux_64bit_8000_b6169.zip"
JAVA_RPM_FILENAME="jdk-7u65-linux-x64.rpm"
JAVA_RPM_URL="http://download.oracle.com/otn-pub/java/jdk/7u65-b17/$JAVA_RPM_FILENAME"

NODE_VERSION="0.10.31"
NODE_TARBALL_DIR="node-v$NODE_VERSION-linux-x64"
NODE_TARBALL_FILENAME="${NODE_TARBALL_DIR}.tar.gz"
NODE_TARBALL_URL="http://nodejs.org/dist/v$NODE_VERSION/$NODE_TARBALL_FILENAME"

set -e

echo "Setting up Centos 6.5"

if ! yum -C repolist | grep epel ; then
  rpm --import https://fedoraproject.org/static/0608B895.txt
  rpm -Uvh http://download-i2.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm
fi

yum -y install gcc-c++ gdb git gperftools gtest gtest-devel htop unzip valgrind yum-utils yum-plugin-auto-update-debug-info.noarch

if [ ! -e /usr/bin/gemfire ]; then
  if [ ! -e /project/tmp/$GEMFIRE_SERVER_FILENAME ]; then
    wget --no-verbose -O /project/tmp/$GEMFIRE_SERVER_FILENAME $S3_PREFIX$GEMFIRE_SERVER_FILENAME
  fi
  rpm -ivh /project/tmp/$GEMFIRE_SERVER_FILENAME
fi

if [ ! -e /opt/pivotal/NativeClient_Linux_64bit_8000_b6169 ]; then
  if [ ! -e /project/tmp/$NATIVE_CLIENT_FILENAME ]; then
    wget --no-verbose -O /project/tmp/$NATIVE_CLIENT_FILENAME $S3_PREFIX$NATIVE_CLIENT_FILENAME
  fi
  cd /opt/pivotal
  unzip /project/tmp/$NATIVE_CLIENT_FILENAME
fi

if [ ! -e /usr/local/bin/node ]; then
  if [ ! -e /project/tmp/$NODE_TARBALL_FILENAME ]; then
    wget --no-verbose -O /project/tmp/$NODE_TARBALL_FILENAME $NODE_TARBALL_URL
  fi
  cd /usr/local
  tar --strip-components 1 -xzf /project/tmp/$NODE_TARBALL_FILENAME
fi

test -e /usr/local/bin/grunt || /usr/local/bin/npm install -g grunt-cli

if [ ! -e /usr/bin/javac ]; then
  if [ ! -e /project/tmp/$JAVA_RPM_FILENAME ]; then
    wget --no-verbose -O /project/tmp/$JAVA_RPM_FILENAME --no-check-certificate --no-cookies --header "Cookie: oraclelicense=accept-securebackup-cookie" $JAVA_RPM_URL
  fi
  rpm -ivh /project/tmp/$JAVA_RPM_FILENAME
fi

sh -c "cat > /etc/profile.d/gfcpp.sh" <<'EOF'
export GFCPP=/opt/pivotal/NativeClient_Linux_64bit_8000_b6169
export GEMFIRE=/opt/pivotal/gemfire/Pivotal_GemFire_800
export JAVA_HOME=/usr/java/default
export PATH=$GFCPP/bin:/usr/local/bin:$PATH
export LD_LIBRARY_PATH=$GFCPP/lib:$LD_LIBRARY_PATH
EOF

wget --no-verbose https://google-styleguide.googlecode.com/svn/trunk/cpplint/cpplint.py -O /usr/local/bin/cpplint.py
chmod +x /usr/local/bin/cpplint.py

if [ ! -e /project/tmp/gppfs-0.2 ]; then
  if [ ! -e /project/tmp/gppfs-0.2.tar.bz2 ]; then
    wget --no-verbose -O /project/tmp/gppfs-0.2.tar.bz2 http://www.joachim-reichel.de/software/gppfs/gppfs-0.2.tar.bz2
  fi
  cd /project/tmp
  tar jxf gppfs-0.2.tar.bz2
fi

sh -c "cat > /home/vagrant/.gdbinit" <<'EOF'
python
import sys

sys.path.insert (0, '/vagrant/tmp/gppfs-0.2')
import stlport.printers
stlport.printers.register_stlport_printers (None)

# see the python module for a description of these options
# stlport.printers.stlport_version           = 5.2
# stlport.printers.print_vector_with_indices = False

end
EOF

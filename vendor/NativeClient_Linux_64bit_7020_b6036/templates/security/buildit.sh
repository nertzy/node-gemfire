#!/bin/bash

PKCSimpl=PkcsAuthInit.cpp
if [ -z ${GFCPP:-} ]; then
  echo set GFCPP...
  exit 1
fi

platform=`uname`
OPENSSL_LINK=
if [ -z ${OPENSSL:-} ]; then
  echo warning: OPENSSL is not set, skipping PKCS AuthInit... PKCS based implementation will not work.
  PKCSimpl=
else
  if [ "$platform" = "SunOS" ]; then
    OPENSSL_LINK="-L${OPENSSL}/lib -R${OPENSSL}/lib"
  else
    OPENSSL_LINK="-L${OPENSSL}/lib -Wl,-rpath,${OPENSSL}/lib"
  fi
fi

OPT=-O3
LIBDIR=lib

if [ "$PKCSimpl" != "" ]; then
  echo compiling with OpenSSL from $OPENSSL
fi

platform=`uname`
is64bit=1
if [ "$platform" == "SunOS" ]; then
  if [ $is64bit -eq 1 ]; then
    ARCH="-xarch=v9"
  else
    ARCH="-xarch=v8plus"
  fi
  CC  -G -Bdynamic \
      -mt -D_RWSTD_MULTI_THREAD -DTHREAD=MULTI \
      -D_REENTRANT -D_EXAMPLE $OPT $ARCH \
      -I$GFCPP/include -I$OPENSSL/include \
      -L$GFCPP/$LIBDIR -R$GFCPP/$LIBDIR ${OPENSSL_LINK} \
      -lgfcppcache -lrt -lcrypto -lpthread -lkstat \
      UserPasswordAuthInit.cpp ${PKCSimpl} -o $GFCPP/$LIBDIR/libsecurityImpl.so
  exitStatus=$?
elif [ "$platform" = "Linux" ]; then
  if [ $is64bit -eq 1 ]; then
    ARCH="-m64"
    FPIC="-fPIC"
  else
    ARCH="-m32"
    FPIC=""
  fi
  g++ -shared \
      -D_REENTRANT -D_EXAMPLE $OPT -Wall $FPIC $ARCH $@\
      -I$GFCPP/include -I$OPENSSL/include \
      -L${GFCPP}/${LIBDIR} -Wl,-rpath,${GFCPP}/${LIBDIR} ${OPENSSL_LINK} \
      -lgfcppcache -lcrypto \
      UserPasswordAuthInit.cpp ${PKCSimpl} -o $GFCPP/$LIBDIR/libsecurityImpl.so
  exitStatus=$?
else
  echo "This script is not supported on this platform."
  exit 1
fi
echo build complete.
exit ${exitStatus}

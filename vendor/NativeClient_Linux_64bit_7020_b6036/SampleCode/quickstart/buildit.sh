#!/bin/bash


if [ -z ${GFCPP:-} ]; then
  echo GFCPP is not set.
  exit 1
fi
if [ -z ${GEMFIRE:-} ]; then
  echo GEMFIRE is not set.
  exit 1
fi

echo Building GemFire QuickStart examples...

OPT=-O3
LIBDIR=lib
is64bit=1
platform=`uname`
if [ "$platform" = "SunOS" ]; then
  if [ $is64bit -eq 1 ]; then
    ARCH="-xarch=v9"
  else
    ARCH="-xarch=v8plus"
  fi
  CC=CC
  CXX_FLAGS="-mt -D_RWSTD_MULTI_THREAD -DTHREAD=MULTI \
      -D_REENTRANT $OPT $ARCH \
      -I$GFCPP/include \
      -L$GFCPP/$LIBDIR \
      -R$GFCPP/$LIBDIR \
      -lgfcppcache -lrt -lpthread -lkstat"
elif [ "$platform" = "Linux" ]; then
  if [ $is64bit -eq 1 ]; then
    ARCH="-m64"
  else
    ARCH="-m32"
  fi
  CC=g++
  CXX_FLAGS="-D_REENTRANT $OPT -Wall $ARCH \
      -I$GFCPP/include \
      -L$GFCPP/$LIBDIR \
      -Wl,-rpath,$GFCPP/$LIBDIR \
      -lgfcppcache"
else
  echo "This script is not supported on this platform."
  exit 1
fi

$CC  $CXX_FLAGS \
    cpp/HACache.cpp -o cpp/HACache
$CC  $CXX_FLAGS \
    cpp/Exceptions.cpp -o cpp/Exceptions    
$CC  $CXX_FLAGS \
    cpp/BasicOperations.cpp -o cpp/BasicOperations 
$CC  $CXX_FLAGS \
    cpp/DistributedSystem.cpp -o cpp/DistributedSystem 
$CC  $CXX_FLAGS \
    cpp/RefIDExample.cpp -o cpp/RefIDExample
$CC  $CXX_FLAGS \
    cpp/PoolWithEndpoints.cpp -o cpp/PoolWithEndpoints 
$CC  $CXX_FLAGS \
    cpp/DataExpiration.cpp \
    cpp/plugins/SimpleCacheListener.cpp -o cpp/DataExpiration 
$CC  $CXX_FLAGS \
    cpp/LoaderListenerWriter.cpp \
    cpp/plugins/SimpleCacheLoader.cpp \
    cpp/plugins/SimpleCacheListener.cpp \
    cpp/plugins/SimpleCacheWriter.cpp -o cpp/LoaderListenerWriter 
$CC  $CXX_FLAGS \
    cpp/DurableClient.cpp \
    cpp/plugins/DurableCacheListener.cpp -o cpp/DurableClient 
$CC  $CXX_FLAGS \
    cpp/RegisterInterest.cpp -o cpp/RegisterInterest 
$CC  $CXX_FLAGS \
    cpp/Security.cpp -o cpp/Security 
$CC  $CXX_FLAGS \
    cpp/MultiuserSecurity.cpp -o cpp/MultiuserSecurity 
$CC  $CXX_FLAGS \
    cpp/RemoteQuery.cpp \
    cpp/queryobjects/Portfolio.cpp \
    cpp/queryobjects/Position.cpp -o cpp/RemoteQuery 
$CC  $CXX_FLAGS \
    cpp/PoolRemoteQuery.cpp \
    cpp/queryobjects/Portfolio.cpp \
    cpp/queryobjects/Position.cpp -o cpp/PoolRemoteQuery 
$CC  $CXX_FLAGS \
    cpp/CqQuery.cpp \
    cpp/queryobjects/Portfolio.cpp \
    cpp/queryobjects/Position.cpp -o cpp/CqQuery
    
$CC  $CXX_FLAGS \
     cpp/PoolCqQuery.cpp \
     cpp/queryobjects/Portfolio.cpp \
     cpp/queryobjects/Position.cpp -o cpp/PoolCqQuery
     
     
$CC  $CXX_FLAGS \
     cpp/Delta.cpp -o cpp/Delta

$CC  $CXX_FLAGS \
    cpp/ExecuteFunctions.cpp  -o cpp/ExecuteFunctions 

$CC  $CXX_FLAGS \
    cpp/PoolCqQuery.cpp \
    cpp/queryobjects/Portfolio.cpp \
    cpp/queryobjects/Position.cpp -o  cpp/PoolCqQuery 
$CC  $CXX_FLAGS \
    interop/InteropCPP.cpp -o interop/InteropCPP 
$CC  $CXX_FLAGS \
    cpp/PutAllGetAllOperations.cpp -o cpp/PutAllGetAllOperations 
$CC  $CXX_FLAGS \
    cpp/Transactions.cpp -o cpp/Transactions
$CC  $CXX_FLAGS \
    cpp/PdxInstance.cpp -o cpp/PdxInstance
$CC  $CXX_FLAGS \
    cpp/PdxSerializer.cpp -o cpp/PdxSerializer	
$CC  $CXX_FLAGS \
    cpp/PdxRemoteQuery.cpp \
    cpp/queryobjects/PortfolioPdx.cpp \
    cpp/queryobjects/PositionPdx.cpp -o cpp/PdxRemoteQuery

javac -classpath $GEMFIRE/lib/gemfire.jar interop/InteropJAVA.java

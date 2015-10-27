#!/bin/bash

PROTOBUF_DIR= Directory where protobuf is installed
OPENSSL_DIR= Directory where OpenSSL is installed
ANDROID_NDK_DIR= Directory where the Android NDK is installed


PROTOBUF_INCLUDE=-I$PROTOBUF_DIR/include/
PROTOBUF_LIBS="$PROTOBUF_DIR/lib/libprotobuf.a $PROTOBUF_DIR/lib/libprotobuf-lite.a"
OPENSSL_INCLUDE=-I$OPENSSL_DIR/include
OPENSSL_LIBS=" ${OPENSSL_DIR}/lib/libssl.a ${OPENSSL_DIR}/lib/libcrypto.a"
ARCH_INCLUDE=-I${ANDROID_NDK_DIR}/platforms/android-14/arch-arm/usr/include/
ARCH_LIBS=-L${ANDROID_NDK_DIR}/platforms/android-14/arch-arm/usr/lib/

CXX="arm-linux-androideabi-g++ --sysroot=$CROSS_SYSROOT"
CXXFLAGS="-I${ANDROID_NDK_DIR}/sources/cxx-stl/gnu-libstdc++/4.7/include/ -I${ANDROID_NDK_DIR}/sources/cxx-stl/gnu-libstdc++/4.7/libs/armeabi/include/"

GNUSTL=$ANDROID_NDK_DIR/sources/cxx-stl/gnu-libstdc++/4.7/libs/armeabi/libgnustl_static.a

set -v
set -x
$CXX $CXXFLAGS -static -pthread -std=c++11 $OPENSSL_INCLUDE $PROTOBUF_INCLUDE $ARCH_INCLUDE $ARCH_LIBS  -o rcast_media_player *.cxx *.cc *.cpp $OPENSSL_LIBS $PROTOBUF_LIBS  $GNUSTL

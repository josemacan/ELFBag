#!/bin/sh

DIR=$(pwd)

export CLANG_TUTOR_DIR=$DIR
export Clang_DIR=/usr/local/llvm11

cd $DIR
mkdir build
cd $DIR/build
cmake -DCT_Clang_INSTALL_DIR=$Clang_DIR $CLANG_TUTOR_DIR


cd $DIR/build
make
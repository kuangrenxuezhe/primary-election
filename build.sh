#!/bin/bash
set +x
DEBUG=""
if [ "$#" -eq 1 ] && [ "$1"  == "debug" ]; then
  DEBUG="debug=1"
fi
cd deps/src/utils
git pull origin master
make clean
make $DEBUG
#make $DEBUG check
make $DEBUG install prefix=../db/deps
make $DEBUG install prefix=../..
cd ../db
git pull origin master
make clean
make $DEBUG
#make $DEBUG check
make $DEBUG with_core=1 install prefix=../..
cd ../../..
git pull origin master
make clean
make $DEBUG
#make $DEBUG check


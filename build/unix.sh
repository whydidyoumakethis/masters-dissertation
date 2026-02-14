#!/bin/sh

if [ -z $1 ]
then
    BUILD_TYPE=Debug
else
    BUILD_TYPE=$1
    shift
fi

if [ -z $1 ]
then
    COMPILER=clang++
else
    COMPILER=$1
    shift
fi

cmake -B output/$BUILD_TYPE -S .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_CXX_COMPILER=$COMPILER
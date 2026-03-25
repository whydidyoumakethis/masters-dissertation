#!/bin/sh

if [ -z $1 ]
then
    COMPILER=clang++
else
    COMPILER=$1
    shift
fi

cmake -B output/Xcode -S .. -G "Xcode" -DCMAKE_CXX_COMPILER=$COMPILER
#!/bin/bash

mkdir -p debug
cd debug
../configure CPPFLAGS="-O0 -std=c++11" CXXFLAGS="-O0" --enable-debug --prefix="" --enable-debug && make ${1} && cp src/dunelegacy ../
cd ..

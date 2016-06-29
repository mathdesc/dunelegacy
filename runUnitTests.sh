#!/bin/bash

mkdir -p build
cd build
../configure CPPFLAGS="-O0 -std=c++11" CXXFLAGS="-O0" --prefix="" --enable-debug 
cd tests
make check
cd ..
cd ..

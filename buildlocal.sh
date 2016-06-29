#!/bin/bash

autoreconf --install
mkdir -p build
cd build
../configure  CPPFLAGS="-std=c++11" --prefix="" && make $1 && cp src/dunelegacy ../
cd ..

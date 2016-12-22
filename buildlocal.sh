#!/bin/bash

autoreconf --install
mkdir -p build
cd build
../configure  CPPFLAGS="-std=c++11 -Wno-narrowing -Wno-unused-but-set-variable -Wno-unused-variable -Wno-format" --prefix="" && make $1 && cp src/dunelegacy ../
cd ..

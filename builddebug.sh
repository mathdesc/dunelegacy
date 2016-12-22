#!/bin/bash

mkdir -p debug
cd debug
../configure CPPFLAGS="-O0 -std=c++11 -Wno-narrowing -Wno-unused-but-set-variable -Wno-unused-variable -Wno-format" CXXFLAGS="-O0" --enable-debug --prefix="" --enable-debug && make ${1} && cp src/dunelegacy ../
cd ..

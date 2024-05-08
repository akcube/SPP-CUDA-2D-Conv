#!/bin/bash
mkdir build
cd build
cmake .. && make -j
cd ..
./build/bin/tester 16384 16384 11111

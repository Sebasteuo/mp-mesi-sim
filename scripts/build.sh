#!/usr/bin/env bash
set -e
mkdir -p build
cd build
cmake .. -DMP_USE_MOCKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build . -- -j 4

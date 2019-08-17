#!/usr/bin/env bash

set -eu
readonly ROOT_PATH=$(cd $(dirname $0) && pwd)
readonly LOCAL_ENV_PATH=${ROOT_PATH}/third_party

mkdir -p build
pushd build

cmake -DLIBWEBRTC_PATH=${LOCAL_ENV_PATH} ..
make

clang++ \
    -O3 \
    -Wall \
    -shared \
    -std=c++11 -undefined \
    dynamic_lookup `python3 -m pybind11 --includes` \
    -o pybind.so \
    -I ../include \
    ../src/pybind.cpp \
    -L../build -lwebrtc

popd

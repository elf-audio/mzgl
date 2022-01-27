#!/bin/zsh
rm -Rf build
mkdir -p build
pushd build
cmake -GXcode .. -DPLATFORM=OS64 -DCMAKE_TOOLCHAIN_FILE=../cmake/ios.toolchain.cmake  -DCPM_SOURCE_CACHE=/tmp
popd

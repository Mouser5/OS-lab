#!/bin/bash

BUILD_DIR="build"

echo "Cleaning previous build..."
rm -rf $BUILD_DIR
mkdir $BUILD_DIR

echo "Building project..."
cd $BUILD_DIR
cmake ..
make

echo "Build complete. Run executables from ./build/"
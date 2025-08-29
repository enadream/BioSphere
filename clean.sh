#!/bin/bash

# Create the build directory if it doesn't exist
if [ ! -d "build" ]; then
  mkdir build
fi

# Navigate to the build directory
cd build

# Run CMake to clean the project
cmake --build . --target clean


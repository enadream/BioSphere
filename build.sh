#!/bin/bash

# Create the build directory if it deosn't exist
if [ ! -d "build" ]; then
  mkdir build
fi

# Navigate the build directory
cd build

# Run CMake and build the project
cmake ..
cmake --build .

# $? is a special variable that holds the exit status of the last command executed
if [ $? -eq 0 ]; then
  echo "Build successful!"
else
  echo "Failed!"
fi

# Wait for an input
# read -p "Press Enter to continue..."

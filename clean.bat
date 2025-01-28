@echo off
REM Create the build dir if it doesn't exits
if not exist build mkdir build

REM Navitate the build directory
cd build

REM Run CMake and build the project
cmake --build . --target clean
pause
@echo off
cls
mkdir build\msvc2022-x84
pushd build\msvc2022-x84
cmake -G "Visual Studio 17" -A Win32 %* ../../neo
popd
@pause
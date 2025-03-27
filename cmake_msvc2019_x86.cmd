@echo off
cls
mkdir build\msvc2019-x86
pushd build\msvc2019-x86
cmake -G "Visual Studio 16" -A Win32 %* ../../neo
popd
@pause
@echo off
cls
mkdir build\msvc2022-x86
pushd build\msvc2022-x86
cmake -G "Visual Studio 17" -A Win32 %* ../../neo
popd
@pause
@echo off
cls
mkdir build\msvc2022-x86_64
pushd build\msvc2022-x86_64
cmake -G "Visual Studio 17" -A x64 %* ../../neo
popd
@pause
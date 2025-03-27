@echo off
cls
mkdir build\msvc2019-x86_64
pushd build\msvc2019-x86_64
cmake -G "Visual Studio 16" -A x64 %* ../../neo
popd
@pause
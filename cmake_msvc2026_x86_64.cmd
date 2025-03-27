@echo off
cls
mkdir build\msvc2026-x86_64
pushd build\msvc2026-x86_64
cmake -G "Visual Studio 18" -A x64 %* ../../neo
popd
@pause
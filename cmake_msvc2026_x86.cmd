@echo off
cls
mkdir build\msvc2026-x86
pushd build\msvc2026-x86
cmake -G "Visual Studio 18" -A Win32 %* ../../neo
popd
@pause
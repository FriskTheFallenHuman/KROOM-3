@echo off
cls

astyle.exe -v --formatted --options=astyle-options.ini --exclude="extern" --recursive *.h
astyle.exe -v --formatted --options=astyle-options.ini --exclude="extern" --exclude="d3xp/gamesys/SysCvar.cpp" --exclude="d3xp/gamesys/Callbacks.cpp" --recursive *.cpp

@pause
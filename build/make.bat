@echo off
mkdir obj\
del /S /Q obj\*
g++ -O3 -std=c++14 -Wall -pedantic -c ..\src\main.cpp -o obj\main.o
g++ -s -o ..\bin\obj2bom.exe obj\main.o
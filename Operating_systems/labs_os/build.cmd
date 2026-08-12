@echo off
echo Обновление кода из Git...
git pull origin main

echo Создание папки build...
if not exist build mkdir build
cd build

echo Генерация Makefile через CMake...
cmake .. -G "MinGW Makefiles"

echo Сборка проекта...
mingw32-make.exe

echo Запуск программы:
hello.exe

pause
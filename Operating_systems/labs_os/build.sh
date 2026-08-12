#!/bin/bash
echo " Обновление кода из Git..."
git pull origin main 2>/dev/null || echo " Удалённый репозиторий не настроен — пропускаем pull."

echo " Создаём папку build..."
mkdir -p build && cd build

echo " Генерируем Makefile через CMake..."
cmake .. && make

echo " Запуск программы:"
./hello

#!/bin/bash

set -e  # если ошибка – сразу выход

# Директория для артефактов
BUILD_DIR=build
mkdir -p $BUILD_DIR

echo "[1/3] Компиляция библиотеки..."
g++ -fPIC -shared lib/cryptography.cpp -o $BUILD_DIR/libcryptography.so -std=c++17 -Wall -O2

echo "[2/3] Компиляция исполняемого файла..."
g++ src/main.cpp -Ilib -L$BUILD_DIR -lcryptography -o $BUILD_DIR/main -std=c++17 -Wall -O2

echo "[3/3] Запуск программы..."
LD_LIBRARY_PATH=$BUILD_DIR $BUILD_DIR/main

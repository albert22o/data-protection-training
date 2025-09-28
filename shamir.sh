#!/bin/bash

# Скрипт для сборки и использования шифра Шамира
# Файл: build.sh

# Директории
SRC_DIR="src"
LIB_DIR="lib"
BUILD_DIR="build"

# Компиляция
compile() {
    echo "Компиляция программы..."
    
    # Создаем директорию для сборки
    mkdir -p $BUILD_DIR
    
    echo "[1/2] Компиляция библиотеки..."
    g++ -fPIC -shared $LIB_DIR/cryptography.cpp -o $BUILD_DIR/libcryptography.so -std=c++17 -Wall -O2
    
    echo "[2/2] Компиляция исполняемого файла..."
    g++ $SRC_DIR/shamir.cpp -I$LIB_DIR -L$BUILD_DIR -lcryptography -o $BUILD_DIR/shamir -std=c++17 -Wall -O2
    
    if [ $? -eq 0 ]; then
        echo "Готово! Исполняемый файл: $BUILD_DIR/shamir"
    else
        echo "Ошибка компиляции!"
        exit 1
    fi
}

# Генерация ключей
genkeys() {
    key_file="${1:-$BUILD_DIR/keys.txt}"
    min_prime="${2:-10000}"
    max_prime="${3:-100000}"
    
    echo "Генерация ключей в файл: $key_file"
    LD_LIBRARY_PATH=$BUILD_DIR $BUILD_DIR/shamir genkeys "$key_file" "$min_prime" "$max_prime"
}

# Шифрование
encrypt() {
    LD_LIBRARY_PATH=$BUILD_DIR $BUILD_DIR/shamir encrypt "$1" "$2" "$3"
}

# Расшифрование
decrypt() {
    LD_LIBRARY_PATH=$BUILD_DIR $BUILD_DIR/shamir decrypt "$1" "$2" "$3"
}

# Очистка
clean() {
    echo "Очистка файлов сборки..."
    rm -rf $BUILD_DIR/*
    echo "Готово!"
}

# Быстрая демонстрация
demo() {
    echo "Демонстрация работы шифра Шамира..."
    compile
    
    # Создаем тестовый файл
    TEST_FILE="$BUILD_DIR/test.txt"
    ENCRYPTED_FILE="$BUILD_DIR/test_encrypted.bin"
    DECRYPTED_FILE="$BUILD_DIR/test_decrypted.txt"
    KEY_FILE="$BUILD_DIR/demo_keys.txt"
    
    echo "Создание тестового файла..."
    echo "Hello, Shamir Cipher!" > "$TEST_FILE"
    echo "This is a test message." >> "$TEST_FILE"
    
    echo "Генерация ключей..."
    genkeys "$KEY_FILE" 500 2000  # Очень маленький диапазон для демо
    
    echo "Шифрование..."
    encrypt "$TEST_FILE" "$ENCRYPTED_FILE" "$KEY_FILE"
    
    echo "Расшифрование..."
    decrypt "$ENCRYPTED_FILE" "$DECRYPTED_FILE" "$KEY_FILE"
    
    echo "Проверка результата..."
    if cmp -s "$TEST_FILE" "$DECRYPTED_FILE"; then
        echo "✅ Демонстрация успешна! Файлы идентичны."
    else
        echo "❌ Ошибка: файлы различаются!"
    fi
    
    echo "Тестовые файлы:"
    echo "  Исходный: $TEST_FILE"
    echo "  Зашифрованный: $ENCRYPTED_FILE"
    echo "  Расшифрованный: $DECRYPTED_FILE"
    echo "  Ключи: $KEY_FILE"
}

# Помощь
help() {
    echo "Использование: $0 [команда]"
    echo ""
    echo "Команды:"
    echo "  compile                    - компилировать программу"
    echo "  genkeys [file] [min] [max] - генерация ключей"
    echo "  encrypt input output keys  - шифрование файла"
    echo "  decrypt input output keys  - расшифрование файла"
    echo "  demo                       - быстрая демонстрация"
    echo "  clean                      - очистить файлы сборки"
    echo ""
    echo "Примеры:"
    echo "  $0 compile"
    echo "  $0 genkeys"
    echo "  $0 genkeys mykeys.txt 500 2000    # быстрая генерация"
    echo "  $0 encrypt text.txt encrypted.bin keys.txt"
    echo "  $0 decrypt encrypted.bin decrypted.txt keys.txt"
    echo "  $0 demo"
    echo "  $0 clean"
}

# Основная логика
case "$1" in
    "compile")
        compile
        ;;
    "genkeys")
        genkeys "$2" "$3" "$4"
        ;;
    "encrypt")
        encrypt "$2" "$3" "$4"
        ;;
    "decrypt")
        decrypt "$2" "$3" "$4"
        ;;
    "demo")
        demo
        ;;
    "clean")
        clean
        ;;
    *)
        help
        ;;
esac

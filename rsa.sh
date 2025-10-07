#!/bin/bash

# Скрипт для сборки и использования шифра RSA
# Файл: build_rsa.sh

# Директории
SRC_DIR="src"
LIB_DIR="lib"
BUILD_DIR="build"

# Компиляция
compile() {
    echo "Компиляция программы RSA..."
    
    mkdir -p $BUILD_DIR
    
    echo "[1/2] Компиляция библиотеки cryptography..."
    g++ -fPIC -shared $LIB_DIR/cryptography.cpp -o $BUILD_DIR/libcryptography.so -std=c++17 -Wall -O2
    
    echo "[2/2] Компиляция программы rsa..."
    g++ $SRC_DIR/rsa.cpp -I$LIB_DIR -L$BUILD_DIR -lcryptography -o $BUILD_DIR/rsa -std=c++17 -Wall -O2
    
    if [ $? -eq 0 ]; then
        echo "✅ Готово! Исполняемый файл: $BUILD_DIR/rsa"
    else
        echo "❌ Ошибка компиляции!"
        exit 1
    fi
}

# Генерация ключей
genkeys() {
    key_file="${1:-$BUILD_DIR/rsa_keys.txt}"
    min_p="${2:-1000}"
    max_p="${3:-10000}"
    
    echo "🔑 Генерация ключей RSA в файл: $key_file"
    LD_LIBRARY_PATH=$BUILD_DIR $BUILD_DIR/rsa genkeys "$key_file" "$min_p" "$max_p"
}

# Шифрование (Алиса)
encrypt() {
    echo "🔒 Шифрование файла..."
    LD_LIBRARY_PATH=$BUILD_DIR $BUILD_DIR/rsa encrypt "$1" "$2" "$3"
}

# Расшифрование (Боб)
decrypt() {
    echo "🔓 Расшифрование файла..."
    LD_LIBRARY_PATH=$BUILD_DIR $BUILD_DIR/rsa decrypt "$1" "$2" "$3"
}

# Очистка
clean() {
    echo "🧹 Очистка файлов сборки..."
    rm -rf $BUILD_DIR/*
    echo "Готово!"
}

# Демонстрация
demo() {
    echo "🚀 Демонстрация работы шифра RSA..."
    compile
    
    TEST_FILE="$BUILD_DIR/test_message.txt"
    ENCRYPTED_FILE="$BUILD_DIR/test_encrypted.bin"
    DECRYPTED_FILE="$BUILD_DIR/test_decrypted.txt"
    KEY_FILE="$BUILD_DIR/demo_keys.txt"
    
    echo "Создание тестового файла..."
    echo "Hello, RSA!" > "$TEST_FILE"
    echo "This is a test message encrypted using RSA algorithm." >> "$TEST_FILE"
    
    echo "Генерация ключей..."
    genkeys "$KEY_FILE" 2000 8000
    
    echo "Шифрование (Алиса → Боб)..."
    encrypt "$TEST_FILE" "$ENCRYPTED_FILE" "$KEY_FILE"
    
    echo "Расшифрование (Боб получает сообщение)..."
    decrypt "$ENCRYPTED_FILE" "$DECRYPTED_FILE" "$KEY_FILE"
    
    echo "Проверка результата..."
    if cmp -s "$TEST_FILE" "$DECRYPTED_FILE"; then
        echo "✅ Демонстрация успешна! Файлы идентичны."
    else
        echo "❌ Ошибка: файлы различаются!"
    fi
    
    echo ""
    echo "📂 Тестовые файлы:"
    echo "  Исходный:     $TEST_FILE"
    echo "  Зашифрованный: $ENCRYPTED_FILE"
    echo "  Расшифрованный: $DECRYPTED_FILE"
    echo "  Ключи:        $KEY_FILE"
}

# Помощь
help() {
    echo "Использование: $0 [команда]"
    echo ""
    echo "Команды:"
    echo "  compile                       - компиляция программы RSA"
    echo "  genkeys [file] [min] [max]    - генерация ключей (P,Q,d,c)"
    echo "  encrypt input output key      - шифрование файла (использует публичный d)"
    echo "  decrypt input output key      - расшифрование файла (использует приватный c)"
    echo "  demo                          - быстрая демонстрация работы RSA"
    echo "  clean                         - очистить build-директорию"
    echo ""
    echo "Примеры:"
    echo "  $0 compile"
    echo "  $0 genkeys"
    echo "  $0 genkeys mykeys.txt 2000 8000"
    echo "  $0 encrypt text.txt encrypted.bin keys.txt"
    echo "  $0 decrypt encrypted.bin decrypted.txt keys.txt"
    echo "  $0 demo"
    echo "  $0 clean"
}

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

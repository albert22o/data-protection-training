#!/bin/bash

# Скрипт для сборки и использования шифра Вернама с генерацией ключа методом Диффи–Хеллмана
# (ключ генерируется под размер файла, который указывает пользователь)

# === Директории ===
SRC_DIR="src"
LIB_DIR="lib"
BUILD_DIR="build"

# === Компиляция ===
compile() {
    echo "Компиляция программы Vernam (Diffie–Hellman)..."

    mkdir -p $BUILD_DIR

    echo "[1/2] Компиляция библиотеки cryptography..."
    g++ -fPIC -shared $LIB_DIR/cryptography.cpp -o $BUILD_DIR/libcryptography.so -std=c++17 -Wall -O2

    echo "[2/2] Компиляция программы vernam..."
    g++ $SRC_DIR/vernam.cpp -I$LIB_DIR -L$BUILD_DIR -lcryptography -o $BUILD_DIR/vernam -std=c++17 -Wall -O2

    if [ $? -eq 0 ]; then
        echo "✅ Готово! Исполняемый файл: $BUILD_DIR/vernam"
    else
        echo "❌ Ошибка компиляции!"
        exit 1
    fi
}

# === Генерация ключей (Diffie–Hellman) под размер указанного файла ===
genkey() {
    key_file="${1:-$BUILD_DIR/vernam_key.bin}"
    target="${2:-}"

    if [ -z "$target" ]; then
        echo "Использование: $0 genkey <key_file> <target_file_or_len>"
        return 1
    fi

    echo "Генерация ключа по протоколу Диффи–Хеллмана под размер: $target"
    LD_LIBRARY_PATH=$BUILD_DIR $BUILD_DIR/vernam genkey "$key_file" "$target"
}

# === Шифрование (Алиса) ===
encrypt() {
    echo "Шифрование файла..."
    LD_LIBRARY_PATH=$BUILD_DIR $BUILD_DIR/vernam encrypt "$1" "$2" "$3"
}

# === Расшифрование (Боб) ===
decrypt() {
    echo "Расшифрование файла..."
    LD_LIBRARY_PATH=$BUILD_DIR $BUILD_DIR/vernam decrypt "$1" "$2" "$3"
}

# === Очистка ===
clean() {
    echo "Очистка файлов сборки..."
    rm -rf $BUILD_DIR/*
    echo "Готово!"
}

# === Демонстрация ===
demo() {
    echo "Демонстрация работы шифра Вернама (DH)..."
    compile

    TEST_FILE="$BUILD_DIR/test_message.txt"
    ENCRYPTED_FILE="$BUILD_DIR/test_encrypted.bin"
    DECRYPTED_FILE="$BUILD_DIR/test_decrypted.txt"
    KEY_FILE="$BUILD_DIR/dh_key.bin"

    echo "Создание тестового файла..."
    echo "Hello, Vernam–Diffie–Hellman!" > "$TEST_FILE"
    echo "This is a test message encrypted using Vernam cipher with DH key generation." >> "$TEST_FILE"

    echo "Генерация ключа (ключ под размер тестового файла)..."
    genkey "$KEY_FILE" "$TEST_FILE"

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

    echo ""
    echo "Тестовые файлы:"
    echo "  Исходный:      $TEST_FILE"
    echo "  Зашифрованный: $ENCRYPTED_FILE"
    echo "  Расшифрованный: $DECRYPTED_FILE"
    echo "  Ключ:          $KEY_FILE"
}

# === Справка ===
help() {
    echo "Использование: $0 [команда]"
    echo ""
    echo "Команды:"
    echo "  compile                         - компиляция программы Vernam (DH)"
    echo "  genkey <key_file> <target>      - генерация ключа методом DH; <target> может быть путем к файлу или числом байт"
    echo "  encrypt input output key        - шифрование файла"
    echo "  decrypt input output key        - расшифрование файла"
    echo "  demo                            - демонстрация работы шифра"
    echo "  clean                           - очистить build-директорию"
    echo ""
    echo "Примеры:"
    echo "  $0 compile"
    echo "  $0 genkey mykey.bin /path/to/file.txt      # ключ сгенерируется под размер файла"
    echo "  $0 genkey mykey.bin 4096                   # ключ длиной 4096 байт"
    echo "  $0 encrypt msg.txt msg.enc mykey.bin"
    echo "  $0 decrypt msg.enc msg_dec.txt mykey.bin"
    echo "  $0 demo"
    echo "  $0 clean"
}

case "$1" in
    "compile")
        compile
        ;;
    "genkey")
        genkey "$2" "$3"
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

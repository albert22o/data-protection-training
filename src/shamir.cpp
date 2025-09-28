#include "../lib/cryptography.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstring>

//! === Вспоомгательные функции для работы с байтами ===

long long bytes_to_long(const std::vector<unsigned char> &bytes)
{
    long long result = 0;
    for (unsigned char byte : bytes)
    {
        result = (result << 8) | byte;
    }
    return result;
}

std::vector<unsigned char> long_to_bytes(long long value, size_t length)
{
    std::vector<unsigned char> result(length);
    for (int i = length - 1; i >= 0; i--)
    {
        result[i] = value & 0xFF;
        value >>= 8;
    }
    return result;
}

//! === Основные функции ===

std::tuple<long long, long long> generate_shamir_keys(long long p)
{
    long long phi = p - 1, c, d;

    while (true)
    {
        // генерируем случайное C взаимно простое с p-1
        c = 2 + (rand() % (phi - 2));
        auto [gcd, x, y] = egcd(c, phi);

        if (gcd == 1)
        {
            d = x % phi;
            if (d < 0)
                d += phi;

            // Проверяем, чт оключи работают правильно
            long long test = 123;
            long long encrypyed = mod_pow(test, c, p);
            long long decrypted = mod_pow(encrypyed, d, p);

            if (decrypted == test)
                return {c, d};
        }
    }
}

void shamir_encrypt(const std::string &input_file, const std::string &output_file,
                    long long p, long long cA, long long dA, long long cB)
{
    std::ifstream in(input_file, std::ios::binary);
    std::ofstream out(output_file, std::ios::binary);

    if (!in)
        throw std::runtime_error("Cannot open input file");
    if (!out)
        throw std::runtime_error("Cannot open output file");

    // Определяем размер блока (максимальное число байт, чтобы значение было < p)
    size_t block_size = 1;
    long long max_value = 255; // 1 байт

    while (max_value * 256 + 255 < p)
    {
        block_size++;
        max_value = max_value * 256 + 255;
    }

    // Записываем размер блока в начало файла
    int block_size_int = static_cast<int>(block_size);
    out.write(reinterpret_cast<const char *>(&block_size_int), sizeof(block_size_int));

    // Обрабатываем файл блоками
    std::vector<unsigned char> buffer(block_size);

    while (in.read(reinterpret_cast<char *>(buffer.data()), block_size) || in.gcount() > 0)
    {
        size_t bytes_read = static_cast<size_t>(in.gcount());

        // Дополняем последний блок нулями если нужно
        if (bytes_read < block_size)
        {
            std::fill(buffer.begin() + bytes_read, buffer.end(), 0);
        }

        // Преобразуем байты в число
        long long m = bytes_to_long(buffer);

        if (m >= p)
        {
            throw std::runtime_error("Message block is too large for prime p");
        }

        // Выполняем протокол Шамира
        long long x1 = mod_pow(m, cA, p);  // Шаг 1: A -> B
        long long x2 = mod_pow(x1, cB, p); // Шаг 2: B -> A
        long long x3 = mod_pow(x2, dA, p); // Шаг 3: A -> B

        // Записываем зашифрованный блок
        auto encrypted_bytes = long_to_bytes(x3, sizeof(long long));
        out.write(reinterpret_cast<const char *>(encrypted_bytes.data()), encrypted_bytes.size());

        // Очищаем буфер для следующего блока
        buffer.assign(block_size, 0);
    }
}

void shamir_decrypt(const std::string &input_file, const std::string &output_file,
                    long long p, long long dB)
{
    std::ifstream in(input_file, std::ios::binary);
    std::ofstream out(output_file, std::ios::binary);

    if (!in)
        throw std::runtime_error("Cannot open input file");
    if (!out)
        throw std::runtime_error("Cannot open output file");

    // Читаем размер блока
    int block_size_int;
    in.read(reinterpret_cast<char *>(&block_size_int), sizeof(block_size_int));
    size_t block_size = static_cast<size_t>(block_size_int);

    // Обрабатываем файл блоками
    std::vector<unsigned char> buffer(sizeof(long long));

    while (in.read(reinterpret_cast<char *>(buffer.data()), sizeof(long long)))
    {
        // Преобразуем байты в число
        long long x3 = bytes_to_long(buffer);

        // Выполняем шаг 4 протокола Шамира
        long long m = mod_pow(x3, dB, p);

        // Преобразуем число обратно в байты
        auto decrypted_bytes = long_to_bytes(m, block_size);

        // Записываем расшифрованные байты
        out.write(reinterpret_cast<const char *>(decrypted_bytes.data()), decrypted_bytes.size());
    }
}

void generate_keys(const std::string &key_file, long long min_prime, long long max_prime)
{
    long long p = generate_prime(min_prime, max_prime);

    // Генерируем ключи для Алисы и Боба
    auto [cA, dA] = generate_shamir_keys(p);
    auto [cB, dB] = generate_shamir_keys(p);

    // Сохраняем ключи в файл
    std::ofstream key_out(key_file);
    key_out << p << std::endl;
    key_out << cA << std::endl;
    key_out << dA << std::endl;
    key_out << cB << std::endl;
    key_out << dB << std::endl;
}

std::tuple<long long, long long, long long, long long, long long> load_keys(const std::string &key_file)
{
    std::ifstream key_in(key_file);
    long long p, cA, dA, cB, dB;
    key_in >> p >> cA >> dA >> cB >> dB;
    return {p, cA, dA, cB, dB};
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage:" << std::endl;
        std::cout << "  " << argv[0] << " genkeys <key_file> [min_prime] [max_prime]" << std::endl;
        std::cout << "  " << argv[0] << " encrypt <input_file> <output_file> <key_file>" << std::endl;
        std::cout << "  " << argv[0] << " decrypt <input_file> <output_file> <key_file>" << std::endl;
        return 1;
    }

    std::string command = argv[1];
    srand(time(0));

    try
    {
        if (command == "genkeys")
        {
            if (argc < 3)
            {
                std::cout << "Usage: " << argv[0] << " genkeys <key_file> [min_prime] [max_prime]" << std::endl;
                return 1;
            }

            long long min_prime = (argc > 3) ? std::stoll(argv[3]) : 1000;
            long long max_prime = (argc > 4) ? std::stoll(argv[4]) : 10000;

            generate_keys(argv[2], min_prime, max_prime);
            std::cout << "Keys generated and saved to " << argv[2] << std::endl;
        }
        else if (command == "encrypt")
        {
            if (argc < 5)
            {
                std::cout << "Usage: " << argv[0] << " encrypt <input_file> <output_file> <key_file>" << std::endl;
                return 1;
            }

            auto [p, cA, dA, cB, dB] = load_keys(argv[4]);
            shamir_encrypt(argv[2], argv[3], p, cA, dA, cB);
            std::cout << "File encrypted successfully" << std::endl;
        }
        else if (command == "decrypt")
        {
            if (argc < 5)
            {
                std::cout << "Usage: " << argv[0] << " decrypt <input_file> <output_file> <key_file>" << std::endl;
                return 1;
            }

            auto [p, cA, dA, cB, dB] = load_keys(argv[4]);
            shamir_decrypt(argv[2], argv[3], p, dB);
            std::cout << "File decrypted successfully" << std::endl;
        }
        else
        {
            std::cout << "Unknown command: " << command << std::endl;
            return 1;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

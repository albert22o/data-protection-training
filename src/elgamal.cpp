#include "cryptography.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstring>
#include <random>
#include <chrono>
#include <tuple>
#include <algorithm>

using ull = unsigned long long;
using ll = long long;

// Запись 64-битного числа в поток в little-endian
static void write_le64(std::ostream &os, ull x)
{
    for (int i = 0; i < 8; ++i)
        os.put(char((x >> (8 * i)) & 0xFF));
}

// Чтение 64-битного числа из потока в little-endian
static ull read_le64(std::istream &is)
{
    ull x = 0;
    for (int i = 0; i < 8; ++i)
    {
        int c = is.get();
        if (c == EOF)
            throw std::runtime_error("Unexpected EOF while read_le64");
        x |= (ull)(unsigned char)c << (8 * i);
    }
    return x;
}

// Преобразует big-endian байтовый вектор в unsigned long long
static ull bytes_to_ull(const std::vector<unsigned char> &bytes)
{
    ull r = 0;
    for (unsigned char b : bytes)
        r = (r << 8) | (ull)b;
    return r;
}

// Преобразует unsigned long long в big-endian байтовый вектор длины out_len
static std::vector<unsigned char> ull_to_bytes(ull v, size_t out_len)
{
    std::vector<unsigned char> res(out_len, 0);
    for (int i = (int)out_len - 1; i >= 0; --i)
    {
        res[i] = (unsigned char)(v & 0xFFu);
        v >>= 8;
    }
    return res;
}

// Возвращает количество байт, необходимых для представления значения x (>0)
static int bytes_needed_for_value(ull x)
{
    int b = 0;
    while (x)
    {
        ++b;
        x >>= 8;
    }
    return b ? b : 1;
}

// Глобальный RNG (инициализация по времени)
static std::mt19937_64 &global_rng()
{
    static std::mt19937_64 rng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());
    return rng;
}

// Возвращает случайное число в диапазоне [lo, hi]
static ull rand_range_ull(ull lo, ull hi)
{
    if (lo > hi)
        std::swap(lo, hi);
    std::uniform_int_distribution<ull> dist(lo, hi);
    return dist(global_rng());
}

// Безопасное умножение по модулю с использованием 128-битного промежуточного результата
static ull modmul_u128(ull a, ull b, ull mod)
{
    __uint128_t t = (__uint128_t)a * (__uint128_t)b;
    return (ull)(t % mod);
}

// Сохранение ключей в текстовый файл: p g d c
static void save_keyfile(const std::string &path, ull p, ull g, ull d, ull c)
{
    std::ofstream f(path);
    if (!f)
        throw std::runtime_error("Cannot write key file");
    f << p << "\n"
      << g << "\n"
      << d << "\n"
      << c << "\n";
}

// Загрузка ключей из файла: p g d c
static std::tuple<ull, ull, ull, ull> load_keyfile(const std::string &path)
{
    std::ifstream f(path);
    if (!f)
        throw std::runtime_error("Cannot open key file");
    ull p = 0, g = 0, d = 0, c = 0;
    f >> p >> g >> d >> c;
    if (!f)
        throw std::runtime_error("Bad key file format");
    return {p, g, d, c};
}

// Формат keyfile: текстовый файл со строками p g d c

// Генерация ключей Эль-Гамаля: генерируется простое p, ищется g, генерируется секретный c, вычисляется d = g^c mod p
void generate_elgamal_keys(const std::string &key_file, ll min_prime, ll max_prime)
{
    ll p = 0;
    ll q = 0;

    while (true)
    {
        ll candidate_q = generate_prime(min_prime, max_prime);
        ll candidate_p = 2 * candidate_q + 1;
        if (candidate_p <= 2)
            continue;

        if (is_probably_prime(candidate_p))
        {
            q = candidate_q;
            p = candidate_p;
            break;
        }
    }

    ll g = 0;

    while (true)
    {
        g = 2 + (rand() % (p - 3));
        if (mod_pow(g, q, p) != 1)
            break;
    }

    ull c = rand_range_ull(2, p - 2);
    ull d = (ull)mod_pow((ll)g, (ll)c, (ll)p); // d = g^c mod p

    save_keyfile(key_file, p, g, d, c);
}

// Формат зашифрованного файла:
// - 4 байта magic "ELG1" (идентификатор формата),
// - 1 байт plain_block — сколько байт исходного сообщения упаковано в блок,
// - 1 байт cipher_block — сколько байт занимает зашифрованный блок e,
// - 8 байт little-endian: p,
// - 8 байт little-endian: orig_size (реальный размер исходного файла),
// - затем для каждого блока:
//  - 8 байт little-endian r,
//  - cipher_block байт big-endian e.

// Шифрование файла: для каждого блока генерируется случайный k, вычисляются r=g^k, s=d^k, e = m * s mod p; записываются r и e
void elgamal_encrypt(const std::string &input_file, const std::string &output_file,
                     ull p, ull g, ull d)
{
    std::ifstream fin(input_file, std::ios::binary);
    std::ofstream fout(output_file, std::ios::binary);
    if (!fin)
        throw std::runtime_error("Cannot open input file");
    if (!fout)
        throw std::runtime_error("Cannot open output file");

    // Определяем размеры блоков
    int pbits = 0;
    for (ull t = p; t; t >>= 1)
        ++pbits;
    size_t plain_block = std::max<size_t>(1, (pbits - 1) / 8);
    size_t cipher_block = std::max<size_t>(1, bytes_needed_for_value(p - 1));

    // Записываем заголовок в файл
    fout.write("ELG1", 4);
    fout.put((char)plain_block);
    fout.put((char)cipher_block);
    write_le64(fout, (ull)p);

    fin.seekg(0, std::ios::end);
    ull orig_size = (ull)fin.tellg();
    fin.seekg(0, std::ios::beg);
    write_le64(fout, orig_size);

    // Чтение и шифрование блоков
    std::vector<unsigned char> inbuf(plain_block);
    while (true)
    {
        fin.read(reinterpret_cast<char *>(inbuf.data()), (std::streamsize)plain_block);
        std::streamsize got = fin.gcount();
        if (got <= 0)
            break;
        // Дополняем блок нулями, если он меньше plain_block
        if ((size_t)got < plain_block)
            std::fill(inbuf.begin() + got, inbuf.end(), 0);

        ull m = bytes_to_ull(inbuf);
        if (m >= p)
            throw std::runtime_error("Message block too large for p");

        ull k = rand_range_ull(1, p - 2);
        ull r = (ull)mod_pow((ll)g, (ll)k, (ll)p); // r = g^k mod p
        ull s = (ull)mod_pow((ll)d, (ll)k, (ll)p); // s = mod_pow(d, k, p);
        ull e = modmul_u128(m, s, p);              // e = (m * s) mod p

        // Записываем в файл r (8 байт LE) и e (cipher_block байт BE)
        write_le64(fout, r);
        auto ebytes = ull_to_bytes(e, cipher_block);
        fout.write(reinterpret_cast<const char *>(ebytes.data()), (std::streamsize)cipher_block);
    }
}

// Расшифровка файла: читаем r и e для каждого блока, вычисляем s = r^c, s^{-1}, m = e * s^{-1} mod p, восстанавливаем исходный размер
void elgamal_decrypt(const std::string &input_file, const std::string &output_file,
                     ull p, ull c_private)
{
    std::ifstream fin(input_file, std::ios::binary);
    std::ofstream fout(output_file, std::ios::binary);
    if (!fin)
        throw std::runtime_error("Cannot open input file");
    if (!fout)
        throw std::runtime_error("Cannot open output file");

    // Читаем заголовок
    char magic[4];
    fin.read(magic, 4);
    if (fin.gcount() != 4 || std::strncmp(magic, "ELG1", 4) != 0)
        throw std::runtime_error("Bad file format (not ELG1)");

    int plain_block = (unsigned char)fin.get();
    int cipher_block = (unsigned char)fin.get();
    ull p_from_file = read_le64(fin);
    ull orig_size = read_le64(fin);
    if (p_from_file != p)
        throw std::runtime_error("Prime p mismatch between key and cipher file");

    // Читаем и расшифровываем блоки
    std::vector<unsigned char> ebuf(cipher_block);
    ull written = 0;
    while (true)
    {
        ull r;
        try
        {
            r = read_le64(fin);
        }
        catch (...)
        {
            break;
        }
        fin.read(reinterpret_cast<char *>(ebuf.data()), cipher_block);
        if (fin.gcount() != cipher_block)
            throw std::runtime_error("Incomplete cipher block");

        ull e = bytes_to_ull(ebuf);
        ull s = (ull)mod_pow((ll)r, (ll)c_private, (ll)p); // s = r^c mod p

        // Находим s_inv по модулю p через расширенный алгоритм Евклида
        auto eg = egcd((ll)s, (ll)p);
        ll g = std::get<0>(eg);
        ll x = std::get<1>(eg);
        if (g != 1)
            throw std::runtime_error("No modular inverse for s (gcd != 1)");
        ll inv = x % (ll)p;
        if (inv < 0)
            inv += (ll)p;
        ull s_inv = (ull)inv;

        ull m = modmul_u128(e, s_inv, p); // m = (e * s_inv) mod p

        // Записываем в файл (учитываем оригинальный размер — обрезаем нули в конце)
        auto outb = ull_to_bytes(m, (size_t)plain_block);
        ull remain = orig_size - written;
        size_t towrite = (size_t)std::min<ull>((ull)plain_block, remain);
        fout.write(reinterpret_cast<const char *>(outb.data()), (std::streamsize)towrite);
        written += towrite;
        if (written >= orig_size)
            break;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage:\n  " << argv[0] << " genkeys <key_file> [min_prime] [max_prime]\n"
                  << "  " << argv[0] << " encrypt <input> <output> <key_file>\n"
                  << "  " << argv[0] << " decrypt <input> <output> <key_file>\n";
        return 1;
    }

    std::string cmd = argv[1];
    try
    {
        if (cmd == "genkeys")
        {
            if (argc < 3)
            {
                std::cerr << "genkeys <key_file> [min] [max]\n";
                return 1;
            }
            ll minp = (argc > 3) ? std::stoll(argv[3]) : 1000;
            ll maxp = (argc > 4) ? std::stoll(argv[4]) : 10000;
            generate_elgamal_keys(argv[2], minp, maxp);
            std::cout << "keys saved to " << argv[2] << "\n";
        }
        else if (cmd == "encrypt")
        {
            if (argc < 5)
            {
                std::cerr << "encrypt <in> <out> <key_file>\n";
                return 1;
            }
            ull p, g, d, c;
            std::tie(p, g, d, c) = load_keyfile(argv[4]);
            elgamal_encrypt(argv[2], argv[3], p, g, d);
            std::cout << "encrypted\n";
        }
        else if (cmd == "decrypt")
        {
            if (argc < 5)
            {
                std::cerr << "decrypt <in> <out> <key_file>\n";
                return 1;
            }
            ull p, g, d, c;
            std::tie(p, g, d, c) = load_keyfile(argv[4]);
            elgamal_decrypt(argv[2], argv[3], p, c);
            std::cout << "decrypted\n";
        }
        else
        {
            std::cerr << "Unknown command\n";
            return 1;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

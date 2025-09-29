#include "../lib/cryptography.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstring>
#include <random>
#include <chrono>
#include <tuple>

using ull = unsigned long long;
using ll = long long;

// --- Вспомогательные: чтение/запись LE 64-bit ---
static void write_le64(std::ostream &os, ull x)
{
    for (int i = 0; i < 8; ++i)
        os.put(char((x >> (8 * i)) & 0xFF));
}
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

// --- Байты <-> число (big-endian) ---
// Внутри используем unsigned long long для безопасности
static ull bytes_to_ull(const std::vector<unsigned char> &bytes)
{
    ull r = 0;
    for (unsigned char b : bytes)
        r = (r << 8) | (ull)b;
    return r;
}
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

// --- подсчёт, сколько байт нужно для представления x (x > 0) ---
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

// --- генерация пары (c,d) для Шамира. Используем mt19937_64 ---
static std::mt19937_64 &global_rng()
{
    static std::mt19937_64 rng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());
    return rng;
}

std::tuple<ll, ll> generate_shamir_keys(ll p)
{
    if (p <= 3)
        throw std::runtime_error("p must be > 3");
    ull phi = (ull)(p - 1);
    std::uniform_int_distribution<ull> dist(2, phi - 1);

    for (int attempts = 0; attempts < 1000000; ++attempts)
    {
        ull c = dist(global_rng());
        // проверяем взаимную простоту
        auto eg = egcd((long long)c, (long long)phi);
        ll g = std::get<0>(eg);
        ll x = std::get<1>(eg);
        if (g != 1)
            continue;
        // d = x mod phi
        ll d = (ll)(x % (ll)phi);
        if (d < 0)
            d += (ll)phi;
        // проверка: c * d % phi == 1
        ull check = (((__int128)c * (ull)d) % phi);
        if (check != 1ULL)
            continue;
        // дополнительная простая проверка на корректность
        ll test = 123;
        ll enc = mod_pow(test, (long long)c, p);
        ll dec = mod_pow(enc, d, p);
        if (dec == test)
            return {(long long)c, d};
    }
    throw std::runtime_error("generate_shamir_keys: cannot find keys");
}

//* Шифрование: используем формат заголовка и согласованные блоки
void shamir_encrypt(const std::string &input_file, const std::string &output_file,
                    ll p, ll cA, ll dA, ll cB)
{
    std::ifstream fin(input_file, std::ios::binary);
    std::ofstream fout(output_file, std::ios::binary);
    if (!fin)
        throw std::runtime_error("Cannot open input file");
    if (!fout)
        throw std::runtime_error("Cannot open output file");

    // plain_block: max байт, чтобы value < p
    int pbits = 0;
    for (ull t = (ull)p; t; t >>= 1)
        ++pbits;
    size_t plain_block = std::max<size_t>(1, (pbits - 1) / 8);
    size_t cipher_block = std::max<size_t>(1, bytes_needed_for_value((ull)(p - 1)));

    // header: "SHAM", plain_block(1), cipher_block(1), p(8LE), orig_size(8LE)
    fout.write("SHAM", 4);
    fout.put((char)plain_block);
    fout.put((char)cipher_block);
    write_le64(fout, (ull)p);
    fin.seekg(0, std::ios::end);
    ull orig_size = (ull)fin.tellg();
    fin.seekg(0, std::ios::beg);
    write_le64(fout, orig_size);

    std::vector<unsigned char> inbuf(plain_block);
    while (true)
    {
        fin.read(reinterpret_cast<char *>(inbuf.data()), (std::streamsize)plain_block);
        std::streamsize got = fin.gcount();
        if (got <= 0)
            break;
        if ((size_t)got < plain_block)
            std::fill(inbuf.begin() + got, inbuf.end(), 0);

        ull m = bytes_to_ull(inbuf);
        if (m >= (ull)p)
            throw std::runtime_error("Message block is too large for prime p");

        // Шаги Шамира
        ll x1 = mod_pow((long long)m, cA, p);
        ll x2 = mod_pow(x1, cB, p);
        ll x3 = mod_pow(x2, dA, p);

        auto enc_bytes = ull_to_bytes((ull)x3, cipher_block);
        fout.write(reinterpret_cast<const char *>(enc_bytes.data()), (std::streamsize)cipher_block);
    }
}

//* Расшифровка: читаем заголовок, применяем dB и возвращаем исходный размер
void shamir_decrypt(const std::string &input_file, const std::string &output_file,
                    ll p, ll dB)
{
    std::ifstream fin(input_file, std::ios::binary);
    std::ofstream fout(output_file, std::ios::binary);
    if (!fin)
        throw std::runtime_error("Cannot open input file");
    if (!fout)
        throw std::runtime_error("Cannot open output file");

    char magic[4];
    fin.read(magic, 4);
    if (fin.gcount() != 4 || std::strncmp(magic, "SHAM", 4) != 0)
        throw std::runtime_error("Bad format");

    int plain_block = (unsigned char)fin.get();
    int cipher_block = (unsigned char)fin.get();
    ll p_from_file = (ll)read_le64(fin);
    ull orig_size = read_le64(fin);
    if (p_from_file != p)
    {
        // можно просто использовать p_from_file, но лучше предупредить
        throw std::runtime_error("Prime p mismatch between key and cipher file");
    }

    std::vector<unsigned char> inbuf(cipher_block);
    ull written = 0;
    while (true)
    {
        fin.read(reinterpret_cast<char *>(inbuf.data()), cipher_block);
        std::streamsize got = fin.gcount();
        if (got <= 0)
            break;
        if ((size_t)got != (size_t)cipher_block)
            throw std::runtime_error("Incomplete cipher block");

        ull x3 = bytes_to_ull(inbuf);
        // шаг 4: apply dB
        ll m_ll = mod_pow((long long)x3, dB, p);
        ull m = (ull)m_ll;
        auto outb = ull_to_bytes(m, (size_t)plain_block);

        // trim if last block
        ull remain = orig_size - written;
        size_t towrite = (size_t)std::min<ull>((ull)plain_block, remain);
        fout.write(reinterpret_cast<const char *>(outb.data()), (std::streamsize)towrite);
        written += towrite;
    }
}

// --- Ключи: генерация и загрузка/сохранение ---
void generate_keys(const std::string &key_file, ll min_prime, ll max_prime)
{
    ll p = generate_prime(min_prime, max_prime); // из вашей библиотеки
    auto [cA, dA] = generate_shamir_keys(p);
    auto [cB, dB] = generate_shamir_keys(p);

    std::ofstream kf(key_file);
    if (!kf)
        throw std::runtime_error("Cannot write keys file");
    kf << p << "\n"
       << cA << "\n"
       << dA << "\n"
       << cB << "\n"
       << dB << "\n";
}

std::tuple<ll, ll, ll, ll, ll> load_keys(const std::string &key_file)
{
    std::ifstream kf(key_file);
    if (!kf)
        throw std::runtime_error("Cannot open keys file");
    ll p, cA, dA, cB, dB;
    kf >> p >> cA >> dA >> cB >> dB;
    if (!kf)
        throw std::runtime_error("Bad keys file format");
    return {p, cA, dA, cB, dB};
}

// --- CLI ---
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
            generate_keys(argv[2], minp, maxp);
            std::cout << "keys saved to " << argv[2] << "\n";
        }
        else if (cmd == "encrypt")
        {
            if (argc < 5)
            {
                std::cerr << "encrypt <in> <out> <key_file>\n";
                return 1;
            }
            auto [p, cA, dA, cB, dB] = load_keys(argv[4]);
            shamir_encrypt(argv[2], argv[3], p, cA, dA, cB);
            std::cout << "encrypted\n";
        }
        else if (cmd == "decrypt")
        {
            if (argc < 5)
            {
                std::cerr << "decrypt <in> <out> <key_file>\n";
                return 1;
            }
            auto [p, cA, dA, cB, dB] = load_keys(argv[4]);
            shamir_decrypt(argv[2], argv[3], p, dB);
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

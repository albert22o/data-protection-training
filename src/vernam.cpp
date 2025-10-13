#include <bits/stdc++.h>
#include "cryptography.h"
#include <filesystem>

using namespace std;
using ll = long long;
using ull = unsigned long long;
namespace fs = std::filesystem;

// --------------------- потоковые утилиты ---------------------
static void write_le64(ostream &os, ull x)
{
    for (int i = 0; i < 8; ++i)
        os.put(static_cast<char>((x >> (8 * i)) & 0xFF));
}
static ull read_le64(istream &is)
{
    ull x = 0;
    for (int i = 0; i < 8; ++i)
    {
        int c = is.get();
        if (c == EOF)
            throw runtime_error("Unexpected EOF while read_le64");
        x |= (ull)(unsigned char)c << (8 * i);
    }
    return x;
}

// --------------------- Vernam + DH key generation ---------------------

// Загрузить ключ-файл целиком в вектор байт
static vector<unsigned char> load_keyfile_bytes(const string &key_file)
{
    ifstream fin(key_file, ios::binary | ios::ate);
    if (!fin)
        throw runtime_error("load_keyfile_bytes: cannot open key file");
    streamsize size = fin.tellg();
    fin.seekg(0, ios::beg);
    if (size < 0)
        throw runtime_error("load_keyfile_bytes: invalid key file size");
    vector<unsigned char> key((size_t)size);
    if (size > 0)
    {
        fin.read(reinterpret_cast<char *>(key.data()), size);
        if (fin.gcount() != size)
            throw runtime_error("load_keyfile_bytes: incomplete read");
    }
    return key;
}

// Простая KDF/PRG: разворачиваем общий секрет K (число) в поток байт требуемой длины.
static vector<unsigned char> expand_secret_to_key(unsigned long long K, unsigned long long key_len)
{
    unsigned char kb[8];
    for (int i = 7; i >= 0; --i)
    {
        kb[i] = (unsigned char)(K & 0xFFu);
        K >>= 8;
    }
    uint32_t seed_data[2];
    seed_data[0] = ((uint32_t)kb[0] << 24) | ((uint32_t)kb[1] << 16) | ((uint32_t)kb[2] << 8) | ((uint32_t)kb[3]);
    seed_data[1] = ((uint32_t)kb[4] << 24) | ((uint32_t)kb[5] << 16) | ((uint32_t)kb[6] << 8) | ((uint32_t)kb[7]);
    std::seed_seq ss(begin(seed_data), end(seed_data));
    std::mt19937_64 gen(ss);

    vector<unsigned char> key;
    key.reserve((size_t)key_len);
    while (key.size() < key_len)
    {
        uint64_t r = gen();
        for (int i = 0; i < 8 && key.size() < key_len; ++i)
        {
            key.push_back(static_cast<unsigned char>(r & 0xFFu));
            r >>= 8;
        }
    }
    return key;
}

// generate_dh_based_key_using_api: использует dh_generate_random_params() и dh_compute_shared()
static void generate_dh_based_key_using_api(const string &key_file, ull key_len)
{
    long long p, g, XA, XB;
    try
    {
        std::tie(p, g, XA, XB) = dh_generate_random_params();
    }
    catch (const std::exception &ex)
    {
        throw runtime_error(string("generate_dh_based_key_using_api: dh_generate_random_params failed: ") + ex.what());
    }

    long long K_ll;
    try
    {
        K_ll = dh_compute_shared(p, g, XA, XB);
    }
    catch (const std::exception &ex)
    {
        throw runtime_error(string("generate_dh_based_key_using_api: dh_compute_shared failed: ") + ex.what());
    }

    if (K_ll <= 0)
        throw runtime_error("generate_dh_based_key_using_api: computed shared key is non-positive");

    unsigned long long K = static_cast<unsigned long long>(K_ll);
    cerr << "DH params (from API): p=" << p << " g=" << g << " XA=" << XA << " XB=" << XB << " K=" << K << "\n";

    vector<unsigned char> key = expand_secret_to_key(K, key_len);

    ofstream fout(key_file, ios::binary);
    if (!fout)
        throw runtime_error("generate_dh_based_key_using_api: cannot open key file for writing");
    fout.write(reinterpret_cast<const char *>(key.data()), (streamsize)key.size());
    if (!fout)
        throw runtime_error("generate_dh_based_key_using_api: write error");
    fout.close();

    cerr << "Generated DH-based key (API) saved to: " << key_file << " (" << key_len << " bytes)\n";
}

// vernam_encrypt: читает input, xor'ит с ключом (из key_file) и пишет результат в output.
// Формат output: "VERN" (4) + orig_size (8 LE) + cipher_bytes
static void vernam_encrypt(const string &input_file, const string &output_file, const string &key_file)
{
    ifstream fin(input_file, ios::binary | ios::ate);
    if (!fin)
        throw runtime_error("vernam_encrypt: cannot open input");
    ull orig_size = (ull)fin.tellg();
    fin.seekg(0, ios::beg);

    vector<unsigned char> key = load_keyfile_bytes(key_file);
    if ((ull)key.size() < orig_size)
        throw runtime_error("vernam_encrypt: key too short for input file (one-time pad requires key length >= message length)");

    ofstream fout(output_file, ios::binary);
    if (!fout)
        throw runtime_error("vernam_encrypt: cannot open output");

    fout.write("VERN", 4);
    write_le64(fout, orig_size);

    const size_t chunk = 1 << 16;
    vector<unsigned char> inbuf(min<ull>(chunk, orig_size));
    ull processed = 0;
    while (processed < orig_size)
    {
        size_t toread = (size_t)min<ull>((ull)inbuf.size(), orig_size - processed);
        fin.read(reinterpret_cast<char *>(inbuf.data()), (streamsize)toread);
        streamsize got = fin.gcount();
        if (got != (streamsize)toread)
            throw runtime_error("vernam_encrypt: read error");
        for (size_t i = 0; i < toread; ++i)
        {
            unsigned char k = key[(size_t)processed + i];
            inbuf[i] ^= k;
        }
        fout.write(reinterpret_cast<const char *>(inbuf.data()), (streamsize)toread);
        if (!fout)
            throw runtime_error("vernam_encrypt: write error");
        processed += toread;
    }
    cerr << "Encryption done: " << output_file << " (" << orig_size << " bytes)\n";
}

// vernam_decrypt: читает cipher файл (формат выше), загружает ключ и выполняет XOR обратно
static void vernam_decrypt(const string &input_file, const string &output_file, const string &key_file)
{
    ifstream fin(input_file, ios::binary);
    if (!fin)
        throw runtime_error("vernam_decrypt: cannot open input");

    char magic[4];
    fin.read(magic, 4);
    if (fin.gcount() != 4 || strncmp(magic, "VERN", 4) != 0)
        throw runtime_error("vernam_decrypt: bad format (missing VERN magic)");

    ull orig_size = read_le64(fin);

    vector<unsigned char> key = load_keyfile_bytes(key_file);
    if ((ull)key.size() < orig_size)
        throw runtime_error("vernam_decrypt: key too short for cipher (cannot decrypt)");

    ofstream fout(output_file, ios::binary);
    if (!fout)
        throw runtime_error("vernam_decrypt: cannot open output");

    const size_t chunk = 1 << 16;
    vector<unsigned char> cbuf(min<ull>(chunk, orig_size));
    ull processed = 0;
    while (processed < orig_size)
    {
        size_t toread = (size_t)min<ull>((ull)cbuf.size(), orig_size - processed);
        fin.read(reinterpret_cast<char *>(cbuf.data()), (streamsize)toread);
        streamsize got = fin.gcount();
        if (got != (streamsize)toread)
            throw runtime_error("vernam_decrypt: incomplete cipher data");
        for (size_t i = 0; i < toread; ++i)
        {
            unsigned char k = key[(size_t)processed + i];
            cbuf[i] ^= k;
        }
        fout.write(reinterpret_cast<const char *>(cbuf.data()), (streamsize)toread);
        if (!fout)
            throw runtime_error("vernam_decrypt: write error");
        processed += toread;
    }
    cerr << "Decryption done: " << output_file << " (" << orig_size << " bytes)\n";
}

// --------------------- main ---------------------
static void print_help_prog(const char *prog)
{
    cerr << "Usage:\n  " << prog << " genkey <key_file> <target_file_or_len>\n"
         << "  " << prog << " encrypt <in> <out> <key_file>\n"
         << "  " << prog << " decrypt <in> <out> <key_file>\n";
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_help_prog(argv[0]);
        return 1;
    }
    string cmd = argv[1];
    try
    {
        if (cmd == "genkey")
        {
            if (argc < 4)
            {
                cerr << "genkey <key_file> <target_file_or_len>\n";
                return 1;
            }
            string key_file = argv[2];
            string target = argv[3];
            unsigned long long key_len = 0;

            // Если target — число, используем его как длину; иначе считаем размер файла
            bool is_number = true;
            for (char c : target)
                if (!isdigit((unsigned char)c))
                {
                    is_number = false;
                    break;
                }

            if (is_number)
            {
                key_len = stoull(target);
            }
            else
            {
                // Получить размер файла
                if (!fs::exists(target))
                {
                    cerr << "genkey: target file does not exist: " << target << "\n";
                    return 1;
                }
                key_len = (unsigned long long)fs::file_size(target);
            }

            if (key_len == 0)
            {
                cerr << "genkey: key length is zero\n";
                return 1;
            }

            generate_dh_based_key_using_api(key_file, key_len);
            cout << "key saved to " << key_file << "\n";
        }
        else if (cmd == "encrypt")
        {
            if (argc < 5)
            {
                cerr << "encrypt <in> <out> <key_file>\n";
                return 1;
            }
            vernam_encrypt(argv[2], argv[3], argv[4]);
            cout << "encrypted\n";
        }
        else if (cmd == "decrypt")
        {
            if (argc < 5)
            {
                cerr << "decrypt <in> <out> <key_file>\n";
                return 1;
            }
            vernam_decrypt(argv[2], argv[3], argv[4]);
            cout << "decrypted\n";
        }
        else
        {
            print_help_prog(argv[0]);
            return 1;
        }
    }
    catch (const exception &ex)
    {
        cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}

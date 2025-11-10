#include <bits/stdc++.h>
#include "cryptography.h"
#include <openssl/sha.h>

using namespace std;
using ull = unsigned long long;
using ll = long long;

// --------------------- вспомогательные функции ---------------------

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
static vector<unsigned char> ull_to_be(ull v, size_t out_len)
{
    vector<unsigned char> res(out_len, 0);
    for (int i = (int)out_len - 1; i >= 0; --i)
    {
        res[i] = (unsigned char)(v & 0xFFu);
        v >>= 8;
    }
    return res;
}
static ull be_to_ull(const vector<unsigned char> &b)
{
    ull r = 0;
    for (unsigned char x : b)
        r = (r << 8) | (ull)x;
    return r;
}
static int bytes_needed(ull x)
{
    int c = 0;
    while (x)
    {
        ++c;
        x >>= 8;
    }
    return c ? c : 1;
}

// вычисление SHA-256 хеша файла
static vector<unsigned char> file_sha256(const string &path)
{
    ifstream in(path, ios::binary);
    if (!in)
        throw runtime_error("Cannot open file for hashing: " + path);

    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    const size_t BUF = 1 << 14;
    vector<char> buf(BUF);
    while (in)
    {
        in.read(buf.data(), (streamsize)BUF);
        streamsize got = in.gcount();
        if (got > 0)
            SHA256_Update(&ctx, buf.data(), (size_t)got);
    }
    vector<unsigned char> digest(SHA256_DIGEST_LENGTH);
    SHA256_Final(digest.data(), &ctx);
    return digest;
}

// обратный элемент по модулю
static ull modinv_via_egcd(ull a, ull mod)
{
    auto t = egcd((long long)a, (long long)mod);
    long long g = get<0>(t), x = get<1>(t);
    if (g != 1)
        throw runtime_error("modinv: inverse does not exist (g != 1)");
    long long r = x % (long long)mod;
    if (r < 0)
        r += (long long)mod;
    return (ull)r;
}

// чтение/запись ключей
static void save_keyfile(const string &path, ll P, ll Q, ll d, ll c)
{
    ofstream f(path);
    if (!f)
        throw runtime_error("Cannot write key file");
    f << P << "\n"
      << Q << "\n"
      << d << "\n"
      << c << "\n";
}
static tuple<ll, ll, ll, ll> load_keyfile(const string &path)
{
    ifstream f(path);
    if (!f)
        throw runtime_error("Cannot open key file");
    ll P = 0, Q = 0, d = 0, c = 0;
    f >> P >> Q >> d >> c;
    if (!f)
        throw runtime_error("Bad key file format");
    return {P, Q, d, c};
}

// --------------------- команды ---------------------

// генерация ключей RSA
static void cmd_genkeys(const string &key_file, ll min_prime, ll max_prime)
{
    // Генерация P и Q (простые)
    ll P = generate_prime(min_prime, max_prime);
    ll Q;
    do
    {
        Q = generate_prime(min_prime, max_prime);
    } while (Q == P);

    // Вычисление N и phi
    ull N = (ull)P * (ull)Q;
    ull phi = (ull)(P - 1) * (ull)(Q - 1);

    // Выбираем d: публичный экспонент (1 < d < phi, gcd(d, phi) == 1)
    ll d = 0;
    std::mt19937_64 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<ull> dist(3ULL, phi > 3 ? phi - 1 : 3ULL);
    for (int i = 0; i < 1000 && d == 0; ++i)
    {
        ull cand = dist(rng);
        if (std::gcd(cand, phi) == 1)
            d = (ll)cand;
    }
    if (d == 0)
        throw runtime_error("generate_rsa_keys: failed to choose d");

    // Вычисляем c = d^{-1} mod phi (закрытый ключ)
    ull c = modinv_via_egcd((ull)d, phi);

    // Сохраняем (P,Q,d,c). P,Q — секрет Боба; d и N — публичные.
    save_keyfile(key_file, P, Q, d, (ll)c);

    cerr << "Generated RSA params: P=" << P << " Q=" << Q << " N=" << N
         << " phi=" << phi << " d=" << d << " c=" << c << "\n";
}

// подпись файла
static void cmd_sign(const string &infile, const string &sigfile, const string &keyfile)
{
    ll P, Q, d_public, c_private;
    tie(P, Q, d_public, c_private) = load_keyfile(keyfile);
    ull N = (ull)P * (ull)Q;

    auto digest = file_sha256(infile);
    ull hash_len = digest.size();

    int Nbits = 0;
    for (ull t = N; t; t >>= 1)
        ++Nbits;
    size_t cipher_block = max<size_t>(1, bytes_needed(N - 1));

    vector<unsigned char> sigdata;
    sigdata.reserve((size_t)hash_len * cipher_block);
    for (size_t i = 0; i < digest.size(); ++i)
    {
        ull m = (ull)digest[i];
        if (m >= N)
            throw runtime_error("hash byte >= N (increase key size)");
        long long s_ll = mod_pow((long long)m, (long long)c_private, (long long)N);
        ull s = (ull)s_ll;
        auto be = ull_to_be(s, cipher_block);
        sigdata.insert(sigdata.end(), be.begin(), be.end());
    }

    ofstream out(sigfile, ios::binary);
    if (!out)
        throw runtime_error("Cannot open signature output: " + sigfile);

    out.write("RSAS", 4);
    out.put(static_cast<char>(cipher_block));
    write_le64(out, N);
    write_le64(out, (ull)hash_len);
    out.write(reinterpret_cast<const char *>(sigdata.data()), (streamsize)sigdata.size());
    out.close();

    cout << "signature written to " << sigfile << "\n";
}

// проверка подписи
static void cmd_verify(const string &infile, const string &sigfile, const string &keyfile)
{
    ll P, Q, d_public, c_private;
    tie(P, Q, d_public, c_private) = load_keyfile(keyfile);
    ull N_from_key = (ull)P * (ull)Q;

    ifstream fin(sigfile, ios::binary);
    if (!fin)
        throw runtime_error("Cannot open signature file: " + sigfile);

    char magic[4];
    fin.read(magic, 4);
    if (fin.gcount() != 4 || strncmp(magic, "RSAS", 4) != 0)
        throw runtime_error("Bad signature format (magic)");

    int cipher_block = (unsigned char)fin.get();
    ull N_in_sig = read_le64(fin);
    ull hash_len = read_le64(fin);

    if (N_in_sig != N_from_key)
        throw runtime_error("Modulus N mismatch between signature and keyfile");

    vector<unsigned char> sigbytes((size_t)hash_len * cipher_block);
    fin.read(reinterpret_cast<char *>(sigbytes.data()), (streamsize)sigbytes.size());
    if ((size_t)fin.gcount() != sigbytes.size())
        throw runtime_error("Incomplete signature data");

    auto digest = file_sha256(infile);
    if (digest.size() != hash_len)
        throw runtime_error("Hash length mismatch (expected in signature)");

    bool ok = true;
    for (size_t i = 0; i < hash_len; ++i)
    {
        vector<unsigned char> block(sigbytes.begin() + i * cipher_block,
                                    sigbytes.begin() + (i + 1) * cipher_block);
        ull s = be_to_ull(block);
        long long w_ll = mod_pow((long long)s, (long long)d_public, (long long)N_from_key);
        ull w = (ull)w_ll;
        if (w != (ull)digest[i])
        {
            ok = false;
            cerr << "Mismatch at byte " << i
                 << ": expected=" << (int)digest[i]
                 << " got=" << (ull)w << "\n";
            break;
        }
    }

    if (ok)
        cout << "Signature is VALID\n";
    else
        cout << "Signature is INVALID\n";
}

// --------------------- main ---------------------

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "Usage:\n"
             << "  " << argv[0] << " genkeys <key_file> [min_prime] [max_prime]\n"
             << "  " << argv[0] << " sign <in_file> <sig_file> <key_file>\n"
             << "  " << argv[0] << " verify <in_file> <sig_file> <key_file>\n";
        return 1;
    }

    try
    {
        string cmd = argv[1];
        if (cmd == "genkeys")
        {
            if (argc < 3)
                throw runtime_error("Usage: genkeys <key_file> [min_prime] [max_prime]");
            ll minp = (argc > 3) ? stoll(argv[3]) : 1000;
            ll maxp = (argc > 4) ? stoll(argv[4]) : 10000;
            cmd_genkeys(argv[2], minp, maxp);
        }
        else if (cmd == "sign")
        {
            if (argc < 5)
                throw runtime_error("Usage: sign <in_file> <sig_file> <key_file>");
            cmd_sign(argv[2], argv[3], argv[4]);
        }
        else if (cmd == "verify")
        {
            if (argc < 5)
                throw runtime_error("Usage: verify <in_file> <sig_file> <key_file>");
            cmd_verify(argv[2], argv[3], argv[4]);
        }
        else
        {
            throw runtime_error("Unknown command");
        }
    }
    catch (const exception &ex)
    {
        cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}

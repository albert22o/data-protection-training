// sign_elgamal.cpp
// Реализация электронной подписи Эль-Гамаля.
// Использует cryptography.h (mod_pow, is_probably_prime, find_generator, egcd, ...)
#include <bits/stdc++.h>
#include "cryptography.h"
#include <openssl/sha.h>

using namespace std;
using ull = unsigned long long;
using ll = long long;

// --- helpers (копия/совместимые с sign_rsa.cpp) ---
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

// --- keyfile format for ElGamal ---
// keyfile (text): p\n g\n y\n x\n
static void save_keyfile(const string &path, ll p, ll g, ll y, ll x)
{
    ofstream f(path);
    if (!f)
        throw runtime_error("Cannot write key file");
    f << p << "\n"
      << g << "\n"
      << y << "\n"
      << x << "\n";
}
static tuple<ll, ll, ll, ll> load_keyfile(const string &path)
{
    ifstream f(path);
    if (!f)
        throw runtime_error("Cannot open key file");
    ll p = 0, g = 0, y = 0, x = 0;
    f >> p >> g >> y >> x;
    if (!f)
        throw runtime_error("Bad key file format");
    return {p, g, y, x};
}

// --- signature file format ---
// binary:
// 4 bytes magic "ELGS"
// then write_le64(p)  (to validate)
// then write_le64(r)
// then write_le64(s)
static void write_sigfile(const string &sigfile, ull p, ull r, ull s)
{
    ofstream out(sigfile, ios::binary);
    if (!out)
        throw runtime_error("Cannot open signature output: " + sigfile);
    out.write("ELGS", 4);
    write_le64(out, p);
    write_le64(out, r);
    write_le64(out, s);
    out.close();
}
static tuple<ull, ull, ull> read_sigfile(const string &sigfile)
{
    ifstream fin(sigfile, ios::binary);
    if (!fin)
        throw runtime_error("Cannot open signature file: " + sigfile);
    char magic[4];
    fin.read(magic, 4);
    if (fin.gcount() != 4 || strncmp(magic, "ELGS", 4) != 0)
        throw runtime_error("Bad signature format (magic)");
    ull p = read_le64(fin);
    ull r = read_le64(fin);
    ull s = read_le64(fin);
    return {p, r, s};
}

// --- commands ---

static void cmd_genkeys(const string &key_file, ll min_prime, ll max_prime)
{
    ll p = generate_prime(min_prime, max_prime); // from cryptography.cpp
    ll g = find_generator(p);
    if (g < 2)
        throw runtime_error("Failed to find generator for p");
    std::mt19937_64 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<ll> dist(2, p - 2);
    ll x = dist(rng); // private key x in [2, p-2]
    ll y = mod_pow(g, x, p);
    save_keyfile(key_file, p, g, y, x);
    cerr << "Generated ElGamal params: p=" << p << " g=" << g << " y=" << y << " x=" << x << "\n";
}

// map SHA256 digest -> integer h in [1, p-1]
static ll digest_to_h(const vector<unsigned char> &digest, ll p)
{
    // interpret digest as big integer modulo (p-1), then add 1 to get into [1,p-1]
    __int128 acc = 0;
    for (unsigned char b : digest)
    {
        acc = (acc << 8) + b;
        // reduce periodically to avoid overflow:
        acc %= ((__int128)(p - 1));
    }
    long long h = (long long)(acc % ((__int128)(p - 1)));
    if (h <= 0)
        h += (p - 1);
    return h; // 1..p-1
}

static void cmd_sign(const string &infile, const string &sigfile, const string &keyfile)
{
    ll p, g, y, x;
    tie(p, g, y, x) = load_keyfile(keyfile);
    if (p < 3)
        throw runtime_error("Bad prime p");

    auto digest = file_sha256(infile);
    ll h = digest_to_h(digest, p); // 1..p-1

    // choose k such that 1<k<p-1 and gcd(k, p-1) == 1
    std::mt19937_64 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<ll> dist(2, p - 2);
    ll k = 0;
    for (int tries = 0; tries < 10000; ++tries)
    {
        ll cand = dist(rng);
        if (std::gcd((long long)cand, (long long)(p - 1)) == 1)
        {
            k = cand;
            break;
        }
    }
    if (k == 0)
        throw runtime_error("Failed to choose k");

    ll r = mod_pow(g, k, p);
    ull k_inv = modinv_via_egcd((ull)k, (ull)(p - 1)); // inverse modulo p-1
    // s = k^{-1} * (h - x*r) mod (p-1)
    long long tmp = (((long long)h - (long long)(((__int128)x * r) % (p - 1))) % (long long)(p - 1));
    if (tmp < 0)
        tmp += (p - 1);
    long long s = ((__int128)k_inv * tmp) % (p - 1);
    if (s < 0)
        s += (p - 1);

    write_sigfile(sigfile, (ull)p, (ull)r, (ull)s);
    cout << "signature written to " << sigfile << "\n";
}

static void cmd_verify(const string &infile, const string &sigfile, const string &keyfile)
{
    ll p_key, g, y, x;
    tie(p_key, g, y, x) = load_keyfile(keyfile);
    ull p_sig, r, s;
    tie(p_sig, r, s) = read_sigfile(sigfile);
    if ((ull)p_key != p_sig)
        throw runtime_error("Prime p mismatch between key and signature");

    auto digest = file_sha256(infile);
    ll h = digest_to_h(digest, p_key);

    if (r < 1 || r > (ull)(p_key - 1))
    {
        cout << "Signature INVALID (r out of range)\n";
        return;
    }
    if (s < 0 || s > (ull)(p_key - 2))
    {
        cout << "Signature INVALID (s out of range)\n";
        return;
    }

    ll left1 = mod_pow(y, (ll)r, p_key);
    ll left2 = mod_pow((ll)r, (ll)s, p_key);
    ll left = ((__int128)left1 * left2) % p_key;
    ll right = mod_pow(g, h, p_key);

    if (left == right)
        cout << "Signature is VALID\n";
    else
        cout << "Signature is INVALID\n";
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "Usage:\n  " << argv[0] << " genkeys <key_file> [min_prime] [max_prime]\n"
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
            throw runtime_error("Unknown command");
    }
    catch (const exception &ex)
    {
        cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}

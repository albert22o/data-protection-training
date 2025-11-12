// sign_gost.cpp
// ГОСТ Р 34.10-94 — учебная реализация (p ~31 bits, q ~16 bits).
// Исправлена формула вычисления s: s = (k*h + x*r) mod q (ГОСТ), а не DSA-style inverse.

#include <bits/stdc++.h>
#include "cryptography.h" // должен содержать mod_pow, is_probably_prime, generate_prime, egcd
#include <openssl/sha.h>
using namespace std;
using ll = long long;

// ---------------- helpers ----------------

static vector<unsigned char> file_sha256(const string &path)
{
    ifstream in(path, ios::binary);
    if (!in)
        throw runtime_error("Cannot open file: " + path);
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    const size_t BUF = 1 << 14;
    vector<char> buf(BUF);
    while (in)
    {
        in.read(buf.data(), BUF);
        streamsize r = in.gcount();
        if (r > 0)
            SHA256_Update(&ctx, buf.data(), (size_t)r);
    }
    vector<unsigned char> dg(SHA256_DIGEST_LENGTH);
    SHA256_Final(dg.data(), &ctx);
    return dg;
}

static tuple<ll, ll, ll> load_params(const string &path)
{
    ifstream f(path);
    if (!f)
        throw runtime_error("Cannot open params");
    ll p, q, a;
    f >> p >> q >> a;
    if (!f)
        throw runtime_error("Bad params file");
    return {p, q, a};
}
static void save_params(const string &path, ll p, ll q, ll a)
{
    ofstream f(path);
    if (!f)
        throw runtime_error("Cannot write params");
    f << p << "\n"
      << q << "\n"
      << a << "\n";
}
static tuple<ll, ll, ll, ll, ll> load_key(const string &path)
{
    ifstream f(path);
    if (!f)
        throw runtime_error("Cannot open key");
    ll p, q, a, x, y;
    f >> p >> q >> a >> x >> y;
    if (!f)
        throw runtime_error("Bad key file");
    return {p, q, a, x, y};
}
static void save_key(const string &path, ll p, ll q, ll a, ll x, ll y)
{
    ofstream f(path);
    if (!f)
        throw runtime_error("Cannot write key");
    f << p << "\n"
      << q << "\n"
      << a << "\n"
      << x << "\n"
      << y << "\n";
}

static void check_params_or_throw(ll p, ll q, ll a)
{
    if (!is_probably_prime(q, 10))
        throw runtime_error("q is not prime");
    if (!is_probably_prime(p, 10))
        throw runtime_error("p is not prime");
    if ((p - 1) % q != 0)
        throw runtime_error("p-1 is not divisible by q (p != b*q+1)");
    if (mod_pow(a, q, p) != 1)
        throw runtime_error("a^q mod p != 1 (a not in subgroup)");
}

static ll mod_mul_safe(ll a, ll b, ll mod)
{
    __int128 t = (__int128)a * (__int128)b;
    t %= mod;
    if (t < 0)
        t += mod;
    return (ll)t;
}

static ll modinv_safe(ll a, ll m)
{
    auto t = egcd(llabs(a), llabs(m));
    ll g = get<0>(t), x = get<1>(t);
    if (g != 1)
        throw runtime_error("modinv: inverse does not exist");
    ll r = x % m;
    if (r < 0)
        r += m;
    return r;
}

// ---------------- genparams ----------------
// generate q (bits_q), find p = b*q + 1 (prime, bits_p), find a = g^b mod p with a>1
static ll generate_prime_bits_mt(int bits, mt19937_64 &rng)
{
    if (bits < 2 || bits > 62)
        throw runtime_error("bits out of range");
    uniform_int_distribution<unsigned long long> dist(0, ULLONG_MAX);
    for (int tries = 0; tries < 200000; ++tries)
    {
        unsigned long long r = dist(rng);
        unsigned long long mask = (bits == 64) ? ~0ULL : ((1ULL << (bits - 1)) - 1ULL);
        unsigned long long cand = (1ULL << (bits - 1)) | (r & mask);
        cand |= 1ULL;
        if (cand < 3)
            continue;
        if (is_probably_prime((ll)cand, 10))
            return (ll)cand;
    }
    throw runtime_error("generate_prime_bits_mt failed");
}

static void cmd_genparams(const string &out_path, int bits_p = 31, int bits_q = 16)
{
    if (bits_q >= bits_p)
        throw runtime_error("bits_q must be < bits_p");
    mt19937_64 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());

    ll q = generate_prime_bits_mt(bits_q, rng);

    // search for p = b*q + 1 with bit length bits_p
    ll p = 0;
    ll b_found = 0;
    long long b_start = 2;
    long long b_end = max(100LL, (1LL << max(1, bits_p - bits_q - 1)));
    bool found = false;
    for (int round = 0; round < 10 && !found; ++round)
    {
        for (long long b = b_start; b <= b_end; ++b)
        {
            __int128 cand = (__int128)b * (__int128)q + 1;
            if (cand > LLONG_MAX)
                break;
            ll pp = (ll)cand;
            int bits = 64 - __builtin_clzll((unsigned long long)pp);
            if (bits != bits_p)
                continue;
            if (is_probably_prime(pp, 10))
            {
                p = pp;
                b_found = b;
                found = true;
                break;
            }
        }
        if (!found)
        {
            b_start = b_end + 1;
            b_end = b_end * 2;
        }
    }
    if (!found)
        throw runtime_error("Failed to find p = b*q + 1");

    // find a = g^b mod p with a>1 (so a^q mod p == 1)
    ll a = 0;
    uniform_int_distribution<long long> dist_g(2, max(2LL, p - 2));
    for (int tries = 0; tries < 200000 && a == 0; ++tries)
    {
        ll g = dist_g(rng);
        ll cand = mod_pow(g, b_found, p);
        if (cand > 1 && mod_pow(cand, q, p) == 1)
        {
            a = cand;
            break;
        }
    }
    if (a == 0)
        throw runtime_error("Failed to find a");

    save_params(out_path, p, q, a);
    cerr << "Generated params: p=" << p << " q=" << q << " a=" << a << " (b=" << b_found << ")\n";
}

// ---------------- genkeys ----------------
static void cmd_genkeys(const string &out_key, const string &params_file)
{
    ll p, q, a;
    tie(p, q, a) = load_params(params_file);
    check_params_or_throw(p, q, a);

    mt19937_64 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());
    uniform_int_distribution<long long> distx(1, q - 1);
    ll x = distx(rng);
    ll y = mod_pow(a, x, p);
    save_key(out_key, p, q, a, x, y);
    cerr << "Generated key: x=" << x << " y=" << y << "\n";
}

// ---------------- sign ----------------
// *** FIX: use GOST formula s = (k*h + x*r) mod q ***
static void cmd_sign(const string &infile, const string &sigfile, const string &keyfile)
{
    ll p, q, a, x, y;
    tie(p, q, a, x, y) = load_key(keyfile);
    check_params_or_throw(p, q, a);

    auto dg = file_sha256(infile);
    ll h = 0;
    for (unsigned char c : dg)
        h = (((__int128)h * 256 + c) % q);
    if (h == 0)
        h = 1;

    mt19937_64 rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());
    uniform_int_distribution<long long> distk(1, q - 1);

    ll r = 0, s = 0;
    for (int tries = 0; tries < 200000; ++tries)
    {
        ll k = distk(rng);
        // require gcd(k,q)==1 not necessary in GOST for this formula, but keep to be safe
        if (std::gcd((long long)k, (long long)q) != 1)
            continue;
        ll ak = mod_pow(a, k, p);
        r = ak % q;
        if (r == 0)
            continue;
        // --- GOST formula (no inverse) ---
        // s = (k * h + x * r) mod q
        __int128 tmp = (__int128)k * (__int128)h + (__int128)x * (__int128)r;
        s = (ll)(tmp % q);
        if (s == 0)
            continue;
        break;
    }
    if (r == 0 || s == 0)
        throw runtime_error("Failed to produce signature");

    ofstream out(sigfile);
    if (!out)
        throw runtime_error("Cannot write sig file");
    out << p << " " << q << " " << a << " " << y << " " << r << " " << s << "\n";
    cout << "Signature written to " << sigfile << "\n";
}

// ---------------- verify ----------------
static void cmd_verify(const string &infile, const string &sigfile, const string &keyfile)
{
    ll kp, kq, ka, kx, ky;
    tie(kp, kq, ka, kx, ky) = load_key(keyfile);

    ifstream fin(sigfile);
    if (!fin)
        throw runtime_error("Cannot open sig");
    ll p, q, a, y, r, s;
    fin >> p >> q >> a >> y >> r >> s;
    if (!fin)
        throw runtime_error("Bad sig file");

    if (p != kp || q != kq || a != ka || y != ky)
    {
        cout << "Signature params mismatch with keyfile\n";
        cout << "Signature is INVALID\n";
        return;
    }
    check_params_or_throw(p, q, a);

    if (!(r > 0 && r < q && s > 0 && s < q))
    {
        cout << "Signature is INVALID (r/s out of range)\n";
        return;
    }

    auto dg = file_sha256(infile);
    ll h = 0;
    for (unsigned char c : dg)
        h = (((__int128)h * 256 + c) % q);
    if (h == 0)
        h = 1;

    ll v = modinv_safe(h, q);
    ll z1 = ((__int128)s * v) % q;
    ll z2 = ((__int128)(q - r) * v) % q;

    ll u1 = mod_pow(a, z1, p);
    ll u2 = mod_pow(y, z2, p);
    ll prod = mod_mul_safe(u1, u2, p);
    ll u = prod % q;

    if (u == r)
        cout << "Signature is VALID\n";
    else
        cout << "Signature is INVALID\n";
}

// ---------------- main ----------------
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cerr << "Usage:\n"
             << "  " << argv[0] << " genparams <params_file> [bits_p bits_q]\n"
             << "  " << argv[0] << " genkeys <key_file> <params_file>\n"
             << "  " << argv[0] << " sign <in_file> <sig_file> <key_file>\n"
             << "  " << argv[0] << " verify <in_file> <sig_file> <key_file>\n";
        return 1;
    }
    try
    {
        string cmd = argv[1];
        if (cmd == "genparams")
        {
            if (argc < 3)
                throw runtime_error("genparams <out> [bits_p bits_q]");
            int bits_p = 31, bits_q = 16;
            if (argc >= 5)
            {
                bits_p = stoi(argv[3]);
                bits_q = stoi(argv[4]);
            }
            cmd_genparams(argv[2], bits_p, bits_q);
        }
        else if (cmd == "genkeys")
        {
            if (argc < 4)
                throw runtime_error("genkeys <keyfile> <paramsfile>");
            cmd_genkeys(argv[2], argv[3]);
        }
        else if (cmd == "sign")
        {
            if (argc < 5)
                throw runtime_error("sign <in> <sig> <keyfile>");
            cmd_sign(argv[2], argv[3], argv[4]);
        }
        else if (cmd == "verify")
        {
            if (argc < 5)
                throw runtime_error("verify <in> <sig> <keyfile>");
            cmd_verify(argv[2], argv[3], argv[4]);
        }
        else
            throw runtime_error("Unknown command");
    }
    catch (const exception &ex)
    {
        cerr << "Error: " << ex.what() << "\n";
        return 2;
    }
    return 0;
}

#include <bits/stdc++.h>
#include "cryptography.h"

using namespace std;
using ll = long long;
using ull = unsigned long long;

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

// --------------------- вспомогательная функция: обратный элемент (через egcd) ---------------------
static ull modinv_via_egcd(ull a, ull mod)
{
    auto t = egcd((long long)a, (long long)mod); // возвращает (g, x, y): a*x + mod*y = g
    long long g = get<0>(t), x = get<1>(t);
    if (g != 1)
        throw runtime_error("modinv_via_egcd: inverse does not exist (g != 1)");
    long long r = x % (long long)mod;
    if (r < 0)
        r += (long long)mod;
    return (ull)r;
}

// --------------------- хранение/загрузка ключей ---------------------
// Формат key_file (текстовый):
//   P
//   Q
//   d (public key)
//   c (private key)
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

// --------------------- основной код (с подробными комментариями) ---------------------

/*
 * generate_rsa_keys
 *
 * Реализует этап инициализации ключей у Боба, в точности по описанному алгоритму:
 * 1) Генерация больших простых P и Q (секретно хранится Бобом).
 * 2) Вычисление N = P * Q и phi = (P-1)*(Q-1).
 * 3) Выбор открытого экспонента d такой, что 1 < d < phi и gcd(d, phi) == 1.
 *    (d — открытый ключ Боба; Алиса использует d при шифровании).
 * 4) Нахождение c — обратного элемента к d по модулю phi: c * d ≡ 1 (mod phi).
 *    (c — закрытый ключ Боба; используется при дешифровании).
 * 5) Сохранение P, Q, d, c в key_file (P и Q — остаются секретом Боба;
 *    d и N — публичные; c — приватный).
 */
static void generate_rsa_keys(const string &key_file, long long min_prime = 1000, long long max_prime = 10000)
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

    cerr << "Generated RSA params (Bob): P=" << P << " Q=" << Q << " N=" << N
         << " phi=" << phi << " d=" << d << " c=" << c << "\n";
}

/*
 * rsa_encrypt
 *
 * Реализует роль Алисы:
 * - Вход: путь к исходному файлу (поток байт), публичные параметры Боба (N и d).
 * - Для каждого блока исходного файла, представленного как целое m (big-endian),
 *   такое что m < N, Алиса вычисляет криптограмму e = m^d mod N (используя mod_pow).
 * - Результат (cipher blocks) записывается в выходной файл в фиксированном формате:
 *   [magic "RSA1"(4)] [plain_block (1)] [cipher_block (1)] [N (8 LE)] [orig_size (8 LE)]
 *   затем последовательность cipher_block байт (big-endian) для каждого зашифрованного блока.
 *
 * Замечания и тонкости:
 * - plain_block выбирается как floor((битовая длина(N)-1)/8) — это гарантирует m < N.
 * - cipher_block = bytes_needed(N-1) — фиксированное число байт для представления c.
 */
static void rsa_encrypt(const string &input_file, const string &output_file, ull N, ll d)
{
    ifstream fin(input_file, ios::binary);
    ofstream fout(output_file, ios::binary);
    if (!fin)
        throw runtime_error("rsa_encrypt: cannot open input");
    if (!fout)
        throw runtime_error("rsa_encrypt: cannot open output");

    // bitlen(N)
    int Nbits = 0;
    for (ull t = N; t; t >>= 1)
        ++Nbits;
    size_t plain_block = max<size_t>(1, (Nbits - 1) / 8);
    size_t cipher_block = max<size_t>(1, bytes_needed(N - 1));

    // Заголовок
    fout.write("RSA1", 4);
    fout.put(static_cast<char>(plain_block));
    fout.put(static_cast<char>(cipher_block));
    write_le64(fout, N);

    fin.seekg(0, ios::end);
    ull orig_size = (ull)fin.tellg();
    fin.seekg(0, ios::beg);
    write_le64(fout, orig_size);

    // Поблочное шифрование
    vector<unsigned char> buf(plain_block);
    while (true)
    {
        fin.read(reinterpret_cast<char *>(buf.data()), (streamsize)plain_block);
        streamsize got = fin.gcount();
        if (got <= 0)
            break;
        if ((size_t)got < plain_block)
            fill(buf.begin() + got, buf.end(), 0);
        ull m = be_to_ull(buf);
        if (m >= N)
            throw runtime_error("rsa_encrypt: message block >= N (increase key size)");
        // e = m^d mod N
        long long m_ll = (long long)m;
        long long N_ll = (long long)N;
        long long e_ll = mod_pow(m_ll, d, N_ll);
        ull e = (ull)e_ll;
        auto ebytes = ull_to_be(e, cipher_block);
        fout.write(reinterpret_cast<const char *>(ebytes.data()), (streamsize)ebytes.size());
    }
}

/*
 * rsa_decrypt
 *
 * Реализует роль Боба:
 * - Вход: зашифрованный файл (созданный rsa_encrypt) и приватный параметр c (закрытый ключ).
 * - Алгоритм: для каждого cipher-block читаем c_big (целое), вычисляем m = c_big^c mod N,
 *   затем переводим m в plain_block байт и записываем в выходной файл, учитывая orig_size
 *   для корректного обрезания последнего блока.
 */
static void rsa_decrypt(const string &input_file, const string &output_file, ull N, ll c)
{
    ifstream fin(input_file, ios::binary);
    ofstream fout(output_file, ios::binary);
    if (!fin)
        throw runtime_error("rsa_decrypt: cannot open input");
    if (!fout)
        throw runtime_error("rsa_decrypt: cannot open output");

    char magic[4];
    fin.read(magic, 4);
    if (fin.gcount() != 4 || strncmp(magic, "RSA1", 4) != 0)
        throw runtime_error("rsa_decrypt: bad format");
    int plain_block = (unsigned char)fin.get();
    int cipher_block = (unsigned char)fin.get();
    ull N_from_file = read_le64(fin);
    ull orig_size = read_le64(fin);
    if (N_from_file != N)
        throw runtime_error("rsa_decrypt: modulus N mismatch");

    vector<unsigned char> cbuf(cipher_block);
    ull written = 0;
    while (true)
    {
        fin.read(reinterpret_cast<char *>(cbuf.data()), cipher_block);
        streamsize got = fin.gcount();
        if (got == 0)
            break;
        if (got != cipher_block)
            throw runtime_error("rsa_decrypt: incomplete cipher block");
        ull e_cipher = be_to_ull(cbuf);
        // m = e_cipher^c mod N
        long long e_ll = (long long)e_cipher;
        long long N_ll = (long long)N;
        long long m_ll = mod_pow(e_ll, c, N_ll);
        ull m = (ull)m_ll;
        auto outb = ull_to_be(m, (size_t)plain_block);
        ull remain = orig_size - written;
        size_t towrite = (size_t)min<ull>((ull)plain_block, remain);
        fout.write(reinterpret_cast<const char *>(outb.data()), (streamsize)towrite);
        written += towrite;
        if (written >= orig_size)
            break;
    }
}

// --------------------- main ---------------------
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "Usage:\n  " << argv[0] << " genkeys <key_file> [min_prime] [max_prime]\n"
             << "  " << argv[0] << " encrypt <in> <out> <key_file>\n"
             << "  " << argv[0] << " decrypt <in> <out> <key_file>\n";
        return 1;
    }
    string cmd = argv[1];
    try
    {
        if (cmd == "genkeys")
        {
            if (argc < 3)
            {
                cerr << "genkeys <key_file> [min_prime] [max_prime]\n";
                return 1;
            }
            long long minp = (argc > 3) ? stoll(argv[3]) : 1000;
            long long maxp = (argc > 4) ? stoll(argv[4]) : 10000;
            generate_rsa_keys(argv[2], minp, maxp);
            cout << "keys saved to " << argv[2] << "\n";
        }
        else if (cmd == "encrypt")
        {
            if (argc < 5)
            {
                cerr << "encrypt <in> <out> <key_file>\n";
                return 1;
            }
            ll P, Q, d, c;
            tie(P, Q, d, c) = load_keyfile(argv[4]);
            ull N = (ull)P * (ull)Q;
            // Alice uses N and d (public) to encrypt
            rsa_encrypt(argv[2], argv[3], N, d);
            cout << "encrypted\n";
        }
        else if (cmd == "decrypt")
        {
            if (argc < 5)
            {
                cerr << "decrypt <in> <out> <key_file>\n";
                return 1;
            }
            ll P, Q, d, c;
            tie(P, Q, d, c) = load_keyfile(argv[4]);
            ull N = (ull)P * (ull)Q;
            // Bob uses N and c (private) to decrypt
            rsa_decrypt(argv[2], argv[3], N, c);
            cout << "decrypted\n";
        }
        else
        {
            cerr << "Unknown command\n";
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

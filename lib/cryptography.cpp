#include "cryptography.h"
#include <stdexcept>
#include <cstdlib>
#include <time.h>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <random>

// Статический генератор псевдослучайных чисел
static std::mt19937_64 CRYPTO_RNG((unsigned)time(nullptr));

// Массив малых простых чисел
static const int SMALL_PRIMES_ARR[] = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
    73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151,
    157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233,
    239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317,
    331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419,
    421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503,
    509, 521, 523, 541, 547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607,
    613, 617, 619, 631, 641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701,
    709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773, 787, 797, 809, 811,
    821, 823, 827, 829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911,
    919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997};
// Размер массива малых простых чисел
static const int SMALL_PRIMES_COUNT = sizeof(SMALL_PRIMES_ARR) / sizeof(SMALL_PRIMES_ARR[0]);

//* Безопасное возведение в степень по модулю
long long mod_pow(long long base, long long exp, long long mod)
{
    if (mod <= 0)
        throw std::invalid_argument("mod_pow: mod must be > 0");
    long long result = 1 % mod;
    long long cur = base % mod;
    if (cur < 0)
        cur += mod;

    while (exp > 0)
    {
        if (exp & 1)
        {
            // используем 128-bit, чтобы избежать переполнения
            __int128 t = (__int128)result * (__int128)cur;
            result = (long long)(t % mod);
        }
        __int128 t2 = (__int128)cur * (__int128)cur;
        cur = (long long)(t2 % mod);
        exp >>= 1;
    }
    return result;
}

//* Тест Миллера-Рабина на простоту + быстрая фильтрация
bool is_probably_prime(long long n, int k)
{
    if (n < 2)
        return false;
    // небольшие простые
    for (int i = 0; i < SMALL_PRIMES_COUNT; ++i)
    {
        int p = SMALL_PRIMES_ARR[i];
        if (n == p)
            return true;
        if (n % p == 0)
            return false;
    }

    // представим n-1 = d * 2^s
    long long d = n - 1;
    int s = 0;
    while ((d & 1) == 0)
    {
        d >>= 1;
        ++s;
    }

    auto try_composite = [&](long long a) -> bool
    {
        long long x = mod_pow(a, d, n);
        if (x == 1 || x == n - 1)
            return false;
        for (int r = 1; r < s; ++r)
        {
            x = mod_pow(x, 2, n);
            if (x == n - 1)
                return false;
            if (x == 1)
                return true; // составное
        }
        return true; // точно составное
    };

    // Если пользователь указал k > 0, исполним k случайных тестов
    if (k > 0)
    {
        std::uniform_int_distribution<unsigned long long> dist(2, (unsigned long long)(n - 2));
        for (int i = 0; i < k; ++i)
        {
            long long a = (long long)dist(CRYPTO_RNG);
            if (try_composite(a))
                return false;
        }
        return true; // вероятно простое
    }

    // k <= 0: используем детерминированный набор оснований, достаточный для 64-bit.
    long long bases[] = {2LL, 325LL, 9375LL, 28178LL, 450775LL, 9780504LL, 1795265022LL};
    for (long long a : bases)
    {
        if (a % n == 0)
            return true;
        if (try_composite(a % n))
            return false;
    }
    return true;
}

//* Генерация случайного простого числа в диапазоне low-high
long long generate_prime(long long low, long long high)
{
    if (low < 2)
        low = 2;
    if (high < low)
        std::swap(low, high);

    // корректируем границы к нечётным и минимум 3
    long long lo = (low % 2 == 0) ? low + 1 : low;
    if (lo < 3)
        lo = 3;
    long long hi = (high % 2 == 0) ? high - 1 : high;
    if (lo > hi)
        throw std::runtime_error("generate_prime: no candidates");

    long long range = (hi - lo) / 2 + 1;
    std::uniform_int_distribution<unsigned long long> dist_idx(0, (unsigned long long)(range - 1));

    const int MAX_RANDOM_TRIES = 2000;
    const int MR_ROUNDS = 7; // количество раундов Miller-Rabin (при k>0)
    for (int attempt = 0; attempt < MAX_RANDOM_TRIES; ++attempt)
    {
        long long idx = (long long)dist_idx(CRYPTO_RNG);
        long long cand = lo + idx * 2;
        // trial division quickly
        bool pass_trial = true;
        for (int i = 0; i < SMALL_PRIMES_COUNT; ++i)
        {
            int p = SMALL_PRIMES_ARR[i];
            if (cand == p)
                return cand;
            if (cand % p == 0)
            {
                pass_trial = false;
                break;
            }
        }
        if (!pass_trial)
            continue;
        if (is_probably_prime(cand, MR_ROUNDS))
            return cand;
    }

    // Если случайное не сработало, делаем последовательный проход
    long long start_idx = dist_idx(CRYPTO_RNG);
    for (long long i = 0; i < range; ++i)
    {
        long long idx = (start_idx + i) % range;
        long long cand = lo + idx * 2;
        bool pass_trial = true;
        for (int j = 0; j < SMALL_PRIMES_COUNT; ++j)
        {
            int p = SMALL_PRIMES_ARR[j];
            if (cand == p)
                return cand;
            if (cand % p == 0)
            {
                pass_trial = false;
                break;
            }
        }
        if (!pass_trial)
            continue;
        if (is_probably_prime(cand, MR_ROUNDS))
            return cand;
    }

    throw std::runtime_error("generate_prime: no prime found in range");
}

long long find_generator(long long p)
{
    if (p <= 2)
        return -1; // для p<=2 нет смысла

    // Опционально: требуем, чтобы p было простым (для простой и корректной работы)
    if (!is_probably_prime(p, 5))
        return -1;

    std::vector<long long> factors;
    long long phi = p - 1;
    long long n = phi;

    // 1. Разложение phi = p-1 на простые множители (без повторов)
    for (long long i = 2; i * i <= n; i++)
    {
        if (n % i == 0)
        {
            factors.push_back(i);
            while (n % i == 0)
                n /= i;
        }
    }
    if (n > 1)
        factors.push_back(n);

    // 2. Перебор кандидатов g
    for (long long g = 2; g < p; g++)
    {
        bool ok = true;
        for (long long f : factors)
        {
            if (mod_pow(g, phi / f, p) == 1)
            {
                ok = false;
                break;
            }
        }
        if (ok)
            return g; // найден генератор
    }

    return -1; // если не нашли
}

std::tuple<long long, long long, long long> egcd(long long a, long long b)
{
    if (a <= 0 || b < 0)
    {
        throw std::invalid_argument(
            "extended_gcd: expected positive a and non-negative b");
    }

    long long u1 = a, u2 = 1, u3 = 0; // U = (u1, u2, u3) = (a, 1, 0)
    long long v1 = b, v2 = 0, v3 = 1; // V = (v1, v2, v3) = (b, 0, 1)

    while (v1 != 0)
    {
        long long q = u1 / v1;
        // T = (u1 mod v1, u2 - q*v2, u3 - q*v3)
        long long t1 = u1 % v1;
        long long t2 = u2 - q * v2;
        long long t3 = u3 - q * v3;

        // Сдвигаем: U = V, V = T
        u1 = v1;
        u2 = v2;
        u3 = v3;
        v1 = t1;
        v2 = t2;
        v3 = t3;
    }

    // Теперь U = (gcd, x, y)
    return {u1, u2, u3};
}

//* Случайная пара чисел (a, b) с условием min_a <= b <= a <= max_a
std::pair<long long, long long> API egcd_generate_random_pair(long long min_a, long long max_a)
{
    if (min_a < 1)
        min_a = 1;
    if (max_a <= min_a)
        max_a = min_a + 1;

    srand(time(0));

    long long a = min_a + (rand() % (max_a - min_a + 1));
    long long b;

    if (a == min_a)
        b = min_a;
    else
        b = min_a + (rand() % (a - min_a + 1));

    return {a, b};
}

//* Генерация пары простых чисел в диапазоне [min_a, max_a]
std::pair<long long, long long> API egcd_generate_prime_pair(long long min_a, long long max_a)
{
    if (min_a < 2)
        min_a = 2;
    if (max_a <= min_a)
        max_a = min_a + 1000;

    long long a = generate_prime(min_a, max_a);

    long long b;
    do
    {
        b = generate_prime(min_a, max_a);
    } while (b == a);

    return {a, b};
}

long long API bsgs(long long a, long long y, long long p)
{
    long long m = (long long)ceil(sqrt(p));
    std::unordered_map<long long, long long> baby_steps;

    // Шаги младенца: y * a^j
    for (long long j = 0; j < m; j++)
    {
        long long value = (y * mod_pow(a, j, p)) % p;
        baby_steps[value] = j;
    }

    long long a_m = mod_pow(a, m, p);

    // Шаги великана: (a^m)^i
    long long gamma = 1;
    for (long long i = 0; i <= m; i++)
    {
        if (baby_steps.count(gamma))
        {
            long long j = baby_steps[gamma];
            long long x = i * m - j;
            if (x >= 0)
                return x % (p - 1);
        }
        gamma = (gamma * a_m) % p;
    }

    return -1; // решение не найдено
}

//* Генерация параметров для задачи дискретного логарифма (BSGS)
//* Возвращает (a, y, p, x) — где y = a^x mod p
std::tuple<long long, long long, long long, long long> API bsgs_generate_random_params(long long min_p, long long max_p)
{
    if (min_p < 3)
        min_p = 3;
    if (max_p <= min_p)
        max_p = min_p + 100;

    srand(time(0));

    long long p = generate_prime(min_p, max_p);
    long long a = find_generator(p);
    long long x = 0;
    if (p > 1)
        x = rand() % (p - 1);
    long long y = mod_pow(a, x, p);

    return {a, y, p, x};
}

long long dh_compute_shared(long long p, long long g, long long XA, long long XB)
{
    if (p <= 2)
        throw std::invalid_argument("dh_compute_shared: p must be > 2");
    if (!(g > 1 && g < p - 1))
        throw std::invalid_argument("dh_compute_shared: g must satisfy 1 < g < p-1");
    if (XA <= 0 || XB <= 0)
        throw std::invalid_argument("dh_compute_shared: secrets must be positive");

    long long YA = mod_pow(g, XA, p);
    long long YB = mod_pow(g, XB, p);

    long long K1 = mod_pow(YB, XA, p); // на стороне A
    long long K2 = mod_pow(YA, XB, p); // на стороне B

    if (K1 != K2)
    {
        throw std::runtime_error("dh_compute_shared: computed keys do not match (possible overflow or bad parameters)");
    }
    return K1;
}

//* Генерация параметров для dh_compute_shared по алгоритму из теории:
//* выбираем p = 2*q + 1, где q — простое (safe prime), затем g такое, что g^q mod p != 1.
//* Возвращает (p, g, XA, XB)
std::tuple<long long, long long, long long, long long> API dh_generate_random_params()
{
    srand(time(0));

    long long p = 0;
    long long q = 0;

    const long long MIN_Q = 500000;
    const long long RANGE_Q = 10000000;

    while (true)
    {
        long long candidate_q = generate_prime(MIN_Q, RANGE_Q);
        long long candidate_p = 2 * candidate_q + 1;
        if (candidate_p <= 2)
            continue;

        if (is_probably_prime(candidate_p))
        {
            q = candidate_q;
            p = candidate_p;
            break;
        }
    }

    long long g = 0;

    while (true)
    {
        g = 2 + (rand() % (p - 3));
        if (mod_pow(g, q, p) != 1)
            break;
    }

    long long XA = 2 + (rand() % (p - 3));
    long long XB = 2 + (rand() % (p - 3));

    return {p, g, XA, XB};
}

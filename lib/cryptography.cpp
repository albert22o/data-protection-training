#include "cryptography.h"
#include <stdexcept>
#include <cstdlib>
#include <time.h>
#include <cmath>
#include <vector>
#include <unordered_map>

long long mod_pow(long long base, long long exp, long long mod)
{
    long long result = 1;
    long long cur = base % mod;

    while (exp > 0)
    {
        if (exp & 1)
        { // если младший бит = 1
            result = (result * cur) % mod;
        }
        cur = (cur * cur) % mod;
        exp >>= 1; // сдвигаем exp на 1 бит влево
    }
    return result;
}

bool is_probably_prime(long long p, int k)
{
    if (p < 4)
        return p == 2 || p == 3; // простые малые числа

    srand(time(0));

    for (int i = 0; i < k; i++)
    {
        long long a = 2 + rand() % (p - 3); // случайное 2 <= a <= p-2
        if (mod_pow(a, p - 1, p) != 1)
        {
            return false; // точно составное
        }
    }
    return true; // вероятно простое
}

long long generate_prime(long long low, long long high)
{
    long long num;

    do
    {
        num = low + rand() % (high - low + 1);
        if (num % 2 == 0)
            num++;
    } while (!is_probably_prime(num));

    return num;
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

std::tuple<long long, long long, long long> extended_gcd(long long a, long long b)
{
    if (a < b)
    {
        throw std::invalid_argument(
            "extended_gcd: expected a >= b, got a < b");
    }
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

    const long long MIN_Q = 5000;
    const long long RANGE_Q = 25000;

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

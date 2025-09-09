#include "cryptography.h"
#include <stdexcept>
#include <cstdlib>
#include <time.h>
#include <cmath>
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

long long API baby_step_giant_step(long long a, long long y, long long p)
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
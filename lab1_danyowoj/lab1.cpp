#include "lab1.h"
#include <stdexcept>
#include <cstdlib>
#include <time.h>

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

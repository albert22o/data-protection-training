#pragma once
#include <tuple>

// Экспорт функций (для Windows нужно __declspec(dllexport/dllimport))
#if defined(_WIN32) || defined(_WIN64)
#ifdef BUILD_DLL
#define API __declspec(dllexport)
#else
#define API __declspec(dllimport)
#endif
#else
#define API
#endif

long long API mod_pow(long long a, long long x, long long p);
bool API is_probably_prime(long long p, int k = 25);
std::tuple<long long, long long, long long> API extended_gcd(long long a, long long b);

long long API baby_step_giant_step(long long a, long long y, long long p);

long long API dh_compute_shared(long long p, long long g, long long XA, long long XB);
std::tuple<long long, long long, long long, long long> API dh_generate_random_params();

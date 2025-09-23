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
long long API generate_prime(long long low, long long high);
long long API find_generator(long long p);

std::tuple<long long, long long, long long> API egcd(long long a, long long b);
std::pair<long long, long long> API egcd_generate_random_pair(long long min_a = 10, long long max_a = 99);
std::pair<long long, long long> API egcd_generate_prime_pair(long long min_a = 10, long long max_a = 99);

long long API bsgs(long long a, long long y, long long p);
std::tuple<long long, long long, long long, long long> API bsgs_generate_random_params(long long min_p = 50, long long max_p = 1000);

long long API dh_compute_shared(long long p, long long g, long long XA, long long XB);
std::tuple<long long, long long, long long, long long> API dh_generate_random_params();

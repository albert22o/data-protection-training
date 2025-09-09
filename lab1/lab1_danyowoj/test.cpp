#include <iostream>
#include "lab1.h"

int main() {
    // === ТЕСТ 1: Расширенный алгоритм Евклида ===
    try {
        auto [g, x, y] = extended_gcd(30, 18);
        std::cout << "[extended_gcd] gcd(30,18) = " << g
                  << " , x = " << x << " , y = " << y << std::endl;
        // Проверка: 30*x + 18*y == gcd
        std::cout << "Проверка: 30*" << x << " + 18*" << y
                  << " = " << 30*x + 18*y << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }

    // === ТЕСТ 2: Быстрое возведение в степень по модулю ===
    long long a = 2, x = 16, p = 17;
    std::cout << "[mod_pow] " << a << "^" << x << " mod " << p
              << " = " << mod_pow(a, x, p) << std::endl; // ожидаем 1

    a = 3; x = 10; p = 11;
    std::cout << "[mod_pow] " << a << "^" << x << " mod " << p
              << " = " << mod_pow(a, x, p) << std::endl; // ожидаем 1

    a = 5; x = 20; p = 23;
    std::cout << "[mod_pow] " << a << "^" << x << " mod " << p
              << " = " << mod_pow(a, x, p) << std::endl; // можно проверить руками/калькулятором

    // === ТЕСТ 3: Проверка простоты (тест Ферма) ===
    long long primes[] = {2, 3, 5, 11, 17, 19, 23, 101};
    long long composites[] = {4, 6, 9, 15, 21, 25, 33, 35, 91};

    std::cout << "\n[is_probably_prime] тестирование простых чисел:\n";
    for (long long p : primes) {
        std::cout << "  " << p << " -> "
                  << (is_probably_prime(p, 5) ? "вероятно простое" : "составное")
                  << std::endl;
    }

    std::cout << "\n[is_probably_prime] тестирование составных чисел:\n";
    for (long long n : composites) {
        std::cout << "  " << n << " -> "
                  << (is_probably_prime(n, 5) ? "вероятно простое" : "составное")
                  << std::endl;
    }

    return 0;
}

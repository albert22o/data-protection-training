#include <iostream>
#include <limits>
#include <stdexcept> // добавлен для invalid_argument
#include "../lib/cryptography.h"

using namespace std;

void waitForAnyKey()
{
    cout << "\nНажмите ENTER для продолжения...";
    cin.clear();                                         // Сбрасываем флаги ошибок
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Очищаем буфер
    cin.get();                                           // Ждем нажатия клавиши
}

int main()
{
    int choice;

    while (true)
    {
        cout << "Выберите программу для теста:\n";
        cout << "1 - Расширенный алгоритм Евклида\n";
        cout << "2 - Быстрое возведение в степень по модулю\n";
        cout << "3 - Проверка простоты (тест Ферма)\n";
        cout << "4 - Поиск дискретного логарифма\n";
        cout << "0 - Выход\n";
        cout << "Ваш выбор: ";
        cin >> choice;

        switch (choice)
        {
        case 1:
        {
            // === ТЕСТ 1: Расширенный алгоритм Евклида ===
            try
            {
                auto [g, x, y] = extended_gcd(30, 18);
                std::cout << "\n[extended_gcd] gcd(30,18) = " << g
                          << " , x = " << x << " , y = " << y << std::endl;
                // Проверка: 30*x + 18*y == gcd
                std::cout << "Проверка: 30*" << x << " + 18*" << y
                          << " = " << 30 * x + 18 * y << std::endl;
            }
            catch (const std::invalid_argument &e)
            {
                std::cerr << "Ошибка: " << e.what() << std::endl;
            }
            waitForAnyKey();
            break;
        }

        case 2:
        {
            // === ТЕСТ 2: Быстрое возведение в степень по модулю ===
            long long a = 2, x = 16, p = 17;
            std::cout << "\n[mod_pow] " << a << "^" << x << " mod " << p
                      << " = " << mod_pow(a, x, p) << std::endl; // ожидаем 1

            a = 3;
            x = 10;
            p = 11;
            std::cout << "[mod_pow] " << a << "^" << x << " mod " << p
                      << " = " << mod_pow(a, x, p) << std::endl; // ожидаем 1

            a = 5;
            x = 20;
            p = 23;
            std::cout << "[mod_pow] " << a << "^" << x << " mod " << p
                      << " = " << mod_pow(a, x, p) << std::endl; // можно проверить руками/калькулятором
            waitForAnyKey();
            break;
        }

        case 3:
        {
            // === ТЕСТ 3: Проверка простоты (тест Ферма) ===
            long long primes[] = {2, 3, 5, 11, 17, 19, 23, 101};
            long long composites[] = {4, 6, 9, 15, 21, 25, 33, 35, 91};

            std::cout << "\n[is_probably_prime] тестирование простых чисел:\n";
            for (long long p : primes)
            {
                std::cout << "  " << p << " -> "
                          << (is_probably_prime(p, 5) ? "вероятно простое" : "составное")
                          << std::endl;
            }

            std::cout << "\n[is_probably_prime] тестирование составных чисел:\n";
            for (long long n : composites)
            {
                std::cout << "  " << n << " -> "
                          << (is_probably_prime(n, 5) ? "вероятно простое" : "составное")
                          << std::endl;
            }
            waitForAnyKey();
            break;
        }

        case 4:
        {
            // === ТЕСТ 4: Поиск дискретного логарифма ===
            struct TestCase
            {
                long long a, y, p, expected;
            };

            TestCase tests[] = {
                {2, 9, 11, 6},   // 2^6 mod 11 = 64 mod 11 = 9
                {3, 10, 13, -1}, // нет такого x, что 3^x mod 13 = 10
                {5, 8, 23, 6},   // 5^6 mod 23 = 8
                {2, 1, 7, 0},    // 2^0 mod 7 = 1
            };

            cout << "\n[Baby-Step Giant-Step] тестирование:\n";

            for (auto t : tests)
            {
                long long result = baby_step_giant_step(t.a, t.y, t.p);
                cout << "a=" << t.a << ", y=" << t.y << ", p=" << t.p
                     << " -> x=" << result << endl;
            }
            waitForAnyKey();
            break;
        }

        case 0:
            cout << "\nВыход из программы.\n";
            return 0;

        default:
            cout << "\nНеверный выбор! Попробуйте снова.\n";
            waitForAnyKey();
            break;
        }
    }
}

#include <iostream>
#include <limits>
#include <stdexcept>
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
        cout << "\nВыберите программу для теста:\n";
        cout << "1 - Расширенный алгоритм Евклида\n";
        cout << "2 - Быстрое возведение в степень по модулю\n";
        cout << "3 - Проверка простоты (тест Ферма)\n";
        cout << "4 - Поиск дискретного логарифма (BSGS)\n";
        cout << "5 - Протокол Диффи-Хеллмана\n";
        cout << "6 - BSGS со случайными параметрами\n";
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
            long long primes[] = {2, 3, 5, 11, 17, 19, 23, 101, 9973};
            long long composites[] = {4, 6, 9, 15, 21, 25, 33, 35, 91, 561}; // 561 - число Кармайкла

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
                {5, 8, 23, 6},   // 5^6 mod 23 = 15625 mod 23 = 8
                {2, 1, 7, 0},    // 2^0 mod 7 = 1
                {6, 7, 41, 9},   // 6^9 mod 41 = 7
            };

            cout << "\n[Baby-Step Giant-Step] тестирование:\n";

            for (auto t : tests)
            {
                long long result = bsgs(t.a, t.y, t.p); 
                cout << "log_" << t.a << "(" << t.y << ") mod " << t.p
                     << " -> x=" << result << " (ожидалось: " << t.expected << ")"
                     << (result == t.expected ? " - OK" : " - FAIL") << endl;
            }
            waitForAnyKey();
            break;
        }

        case 5:
        {
            cout << "\n[Diffie-Hellman] генерация случайных параметров...\n";
            try
            {
                auto [p, g, XA, XB] = dh_generate_random_params();
                cout << "  p = " << p << endl;
                cout << "  g = " << g << endl;
                cout << "  Секрет Алисы (XA): " << XA << endl;
                cout << "  Секрет Боба (XB): " << XB << endl;

                cout << "\nВычисление общего ключа...\n";
                long long shared_key = dh_compute_shared(p, g, XA, XB);
                cout << "  Общий секретный ключ: " << shared_key << endl;

                long long YA = mod_pow(g, XA, p);
                long long YB = mod_pow(g, XB, p);
                long long K1 = mod_pow(YB, XA, p);
                long long K2 = mod_pow(YA, XB, p);
                cout << "  Проверка: K1 (Алиса) = " << K1 << ", K2 (Боб) = " << K2
                     << (K1 == shared_key && K2 == shared_key ? " - OK" : " - FAIL") << endl;
            }
            catch (const std::runtime_error &e)
            {
                std::cerr << "Ошибка выполнения: " << e.what() << std::endl;
            }
            waitForAnyKey();
            break;
        }

        case 6:
        {
            cout << "\n[BSGS Random] генерация случайной задачи...\n";
            try
            {
                auto [a, y, p, x_expected] = bsgs_generate_random_params(10000, 50000);
                cout << "  Решаем уравнение: " << a << "^x = " << y << " (mod " << p << ")\n";
                cout << "  (Заранее известный ответ: x = " << x_expected << ")\n";

                long long x_found = bsgs(a, y, p);
                cout << "  Найденный результат: x = " << x_found << endl;
                cout << "  Проверка: " << (x_found == x_expected ? "УСПЕХ" : "НЕУДАЧА") << endl;
            }
            catch (const std::runtime_error &e)
            {
                std::cerr << "Ошибка выполнения: " << e.what() << std::endl;
            }
            waitForAnyKey();
            break;
        }

        case 0:
            cout << "\nВыход из программы.\n";
            return 0;

        default:
            cout << "\nНеверный выбор! Попробуйте снова.\n";
            if (cin.fail())
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
            }
            waitForAnyKey();
            break;
        }
    }
}
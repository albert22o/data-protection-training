#include <iostream>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <vector>
#include "../lib/cryptography.h"

using namespace std;

long long generate_prime(long long low, long long high)
{
    long long num;
    do
    {
        num = low + rand() % (high - low + 1);
    } while (!is_probably_prime(num));
    return num;
}

void waitForAnyKey()
{
    cout << "\nНажмите ENTER для продолжения...";
    cin.clear();                                         // Сбрасываем флаги ошибок
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Очищаем буфер
    cin.get();                                           // Ждем нажатия клавиши
}

long long find_generator(long long p)
{
    vector<long long> factors;
    long long phi = p - 1, n = phi;

    // 1. Разложение p-1 на простые множители
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
        // Проверка условия: g^(phi/f) != 1 (mod p) для всех простых делителей f числа phi
        for (long long f : factors)
        {
            if (mod_pow(g, phi / f, p) == 1)
            {
                ok = false;
                break;
            }
        }
        if (ok)
            return g; // нашли генератор
    }
    return -1; // если вдруг не нашли
}

int main()
{
    srand(time(0));

    long long a, b, c;
    int choice;
    int subchoice;

    while (true)
    {
        cout << "\nМеню:\n";
        cout << "1 - Возведение в степень по модулю\n";
        cout << "2 - Проверка числа на простоту\n";
        cout << "3 - Обобщенный алгоритм Евклида\n";
        cout << "4 - Поиск дискретного логарифма\n";
        cout << "5 - Построение общего ключа по схеме Диффи-Хеллмана\n";
        cout << "0 - Выход\n";
        cout << "Ваш выбор: ";
        cin >> choice;

        switch (choice)
        {
        //! Возведение в степень по модулю
        case 1:
            cout << "\nВведите a, x, p (a^x mod p): ";
            cin >> a >> b >> c;
            cout << "Результат: " << mod_pow(a, b, c) << endl;

            waitForAnyKey();
            break;
        //! Проверка числа на простоту
        case 2:
            cout << "\nВведите число для проверки: ";
            cin >> a;
            if (is_probably_prime(a))
                cout << "Число предположительно простое" << endl;
            else
                cout << "Число составное" << endl;

            waitForAnyKey();
            break;
        //! Обобщенный алгоритм Евклида
        case 3:
        {
            cout << "\nВыберите способ ввода чисел:" << endl;
            cout << "1 - Ввод с клавиатуры" << endl;
            cout << "2 - Генерация случайных чисел" << endl;
            cout << "3 - Генерация случайных простых чисел" << endl;
            cout << "Ваш выбор: ";
            cin >> subchoice;

            switch (subchoice)
            {
            //* Ввод с клавиатуры
            case 1:
            {
                while (true)
                {
                    cout << "\nВведите a и b (a >= b): ";
                    cin >> a >> b;
                    if (a >= b)
                        break;
                    cout << "Число a должно быть больше или равно b. Попробуйте снова.\n";
                }

                auto [g, x, y] = extended_gcd(a, b);
                cout << "НОД(" << a << ", " << b << ") = " << g << endl;
                cout << "x = " << x << ", y = " << y << endl;
                cout << a << "*" << x << " + " << b << "*" << y << " = " << g << endl;

                waitForAnyKey();
                break;
            }
            //* Случайные числа
            case 2:
            {
                a = 10 + rand() % 90;
                do
                {
                    b = 10 + rand() % a;
                } while (b > a);
                cout << "\nСгенерированы числа: a = " << a << ", b = " << b << endl;

                auto [g, x, y] = extended_gcd(a, b);
                cout << "НОД(" << a << ", " << b << ") = " << g << endl;
                cout << "x = " << x << ", y = " << y << endl;
                cout << a << "*" << x << " + " << b << "*" << y << " = " << g << endl;

                waitForAnyKey();
                break;
            }
            //* Случайные простые числа
            case 3:
            {
                a = generate_prime(10, 200);
                do
                {
                    b = generate_prime(10, a);
                } while (b > a);
                cout << "\nСгенерированы простые числа: a = " << a << ", b = " << b << endl;

                auto [g, x, y] = extended_gcd(a, b);
                cout << "НОД(" << a << ", " << b << ") = " << g << endl;
                cout << "x = " << x << ", y = " << y << endl;
                cout << a << "*" << x << " + " << b << "*" << y << " = " << g << endl;

                waitForAnyKey();
                break;
            }
            //* Ошибка
            default:
                cout << "Неверный выбор!\n";
                break;
            }

            break;
        }
        //! Поиск дискретного логарифма
        case 4:
        {
            cout << "\nВыберите способ ввода чисел:" << endl;
            cout << "1 - Ввод с клавиатуры" << endl;
            cout << "2 - Генерация случайных чисел" << endl;
            cout << "Ваш выбор: ";
            cin >> subchoice;

            switch (subchoice)
            {
            //* Ввод с клавиатуры
            case 1:
            {
                cout << "\nВведите a, y, p (y = a^x mod p): ";
                cin >> a >> b >> c;
                long long x = baby_step_giant_step(a, b, c);
                if (x != -1)
                    cout << "Решение: x = " << x << endl;
                else
                    cout << "Решение не найдено" << endl;

                waitForAnyKey();
                break;
            }
            //* Случайные числа
            case 2:
            {
                c = generate_prime(50, 1000);
                a = find_generator(c);
                long long x = rand() % (c - 1);
                b = mod_pow(a, x, c);

                cout << "\nСгенерированы числа: a = " << a << ", y = " << b << ", p = " << c << endl;
                x = baby_step_giant_step(a, b, c);
                if (x != -1)
                    cout << "Решение: x = " << x << endl;
                else
                    cout << "Решение не найдено" << endl;

                waitForAnyKey();
                break;
            }
            //* Ошибка
            default:
                cout << "Неверный выбор!\n";
                break;
            }

            break;
        }
        //! Поиск общего ключа по схеме Деффи-Хеллмана
        case 5:
        {
            cout << "\nВыберите способ ввода чисел:" << endl;
            cout << "1 - Ввод с клавиатуры" << endl;
            cout << "2 - Генерация случайных чисел" << endl;
            cout << "Ваш выбор: ";
            cin >> subchoice;

            switch (subchoice)
            {
            //* Ввод с клавиатуры
            case 1:
            {
                long long p, g, XA, XB;
                cout << "\nВведите p, g, XA, XB: ";
                cin >> p >> g >> XA >> XB;
                cout << "Общий ключ K для двух абонентов равен: " << dh_compute_shared(p, g, XA, XB) << endl;
                //* При вводе 23 5 7 13 ответ должен быть 10
                waitForAnyKey();
                break;
            }
            //* Случайные числа
            case 2:
            {
                auto [p, g, XA, XB] = dh_generate_random_params();
                cout << "\nСгенерированные параметры: p = " << p << ", g = " << g << ", XA = " << XA << ", XB = " << XB << endl;
                cout << "Общий ключ K для двух абонентов равен: " << dh_compute_shared(p, g, XA, XB) << endl;
                waitForAnyKey();
                break;
            }
            //* Ошибка
            default:
                cout << "Неверный выбор!\n";
                break;
            }

            break;
        }

        //! Выход
        case 0:
            cout << "\nВыход из программы.\n";
            return 0;
        //! Ошибка
        default:
            cout << "Неверный выбор! Попробуйте снова.\n";
        }
    }
}

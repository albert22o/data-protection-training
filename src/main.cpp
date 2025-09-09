#include <iostream>
#include <cstdlib>
#include <ctime>
#include <limits>
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

int main()
{
    srand(time(0));

    long long a, b, c;
    int choice;

    while (true)
    {
        cout << "\nМеню:\n";
        cout << "1 - Ввод чисел a и b с клавиатуры\n";
        cout << "2 - Генерация случайных чисел\n";
        cout << "3 - Генерация случайных простых чисел\n";
        cout << "4 - Проверка числа на простоту\n";
        cout << "5 - Возведение по модулю\n";
        cout << "0 - Выход\n";
        cout << "Ваш выбор: ";
        cin >> choice;

        switch (choice)
        {
        case 1:
            while (true)
            {
                cout << "\nВведите a и b (a >= b): ";
                cin >> a >> b;
                if (a >= b)
                    break;
                cout << "Число a должно быть больше или равно b. Попробуйте снова.\n";
            }
            {
                auto [g, x, y] = extended_gcd(a, b);
                cout << "НОД(" << a << ", " << b << ") = " << g << endl;
                cout << "x = " << x << ", y = " << y << endl;
                cout << a << "*" << x << " + " << b << "*" << y << " = " << g << endl;
            }
            waitForAnyKey();
            break;

        case 2:
            a = 10 + rand() % 90;
            do
            {
                b = 10 + rand() % a;
            } while (b > a);
            cout << "\nСгенерированы числа: a = " << a << ", b = " << b << endl;
            {
                auto [g, x, y] = extended_gcd(a, b);
                cout << "НОД(" << a << ", " << b << ") = " << g << endl;
                cout << "x = " << x << ", y = " << y << endl;
                cout << a << "*" << x << " + " << b << "*" << y << " = " << g << endl;
            }
            waitForAnyKey();
            break;

        case 3:
            a = generate_prime(10, 200);
            do
            {
                b = generate_prime(10, a);
            } while (b > a);
            cout << "\nСгенерированы простые числа: a = " << a << ", b = " << b << endl;
            {
                auto [g, x, y] = extended_gcd(a, b);
                cout << "НОД(" << a << ", " << b << ") = " << g << endl;
                cout << "x = " << x << ", y = " << y << endl;
                cout << a << "*" << x << " + " << b << "*" << y << " = " << g << endl;
            }
            waitForAnyKey();
            break;

        case 4:
            cout << "\nВведите число для проверки: ";
            cin >> a;
            if (is_probably_prime(a))
                cout << "Число предположительно простое" << endl;
            else
                cout << "Число составное" << endl;
            waitForAnyKey();
            break;

        case 5:
            cout << "\nВведите основание a, степень b и mod: ";
            cin >> a >> b >> c;
            cout << "Результат: " << mod_pow(a, b, c) << endl;
            waitForAnyKey();
            break;

        case 0:
            cout << "\nВыход из программы.\n";
            return 0;

        default:
            cout << "Неверный выбор! Попробуйте снова.\n";
        }
    }
}

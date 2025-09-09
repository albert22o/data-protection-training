/* 
*Сборки библиотеки на Linux:
g++ -fPIC -DBUILD_DLL -shared lab1.cpp -o liblab1.so

*Запуск на Linux:
g++ main.cpp -L. -llab1 -o main && ./main
*/
#include <iostream>
#include "lab1.h"

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

int main()
{
    srand(time(0));

    long long a, b, c;
    int choice;
    cout << "Выберите способ задания чисел a и b:\n";
    cout << "1 - Ввод с клавиатуры\n";
    cout << "2 - Генерация случайных чисел\n";
    cout << "3 - Генерация случайных простых чисел\n";
    cout << "4 - Проверка числа на простоту\n";
    cout << "5 - Возведение по модулю\n";
    cout << "Ваш выбор: ";
    cin >> choice;

    if (choice == 1)
    {
        while (true)
        {
            cout << "Введите a и b (a >= b): ";
            cin >> a >> b;
            if (a >= b)
                break;
            cout << "Число a должно быть больше или равно b. Попробуйте снова.\n";
        }
    }
    else if (choice == 2)
    {
        a = 10 + rand() % 90;
        do
        {
            b = 10 + rand() % a;
        } while (b > a);
        cout << "Сгенерированы числа: a = " << a << ", b = " << b << endl;
    }
    else if (choice == 3)
    {
        a = generate_prime(10, 200);
        do
        {
            b = generate_prime(10, a);
        } while (b > a);
        cout << "Сгенерированы простые числа: a = " << a << ", b = " << b << endl;
    }
    else if (choice == 4){
        cout << "Введите число для проверки: ";
        cin >> a;
        if (is_probably_prime(a)){
            cout << "Число предположительно простое" << endl;
        }
        else{
            cout << "Число составное" << endl;
        }
        return 0;
    }
    else if (choice == 5){
            cout << "Введите основание a, степень b и mod: ";
        cin >> a >> b >> c;
        cout << "Модуль: " << mod_pow(a,b,c) << endl;
        return 0;
    }
    else
    {
        cout << "Неверный выбор!" << endl;
        return 0;
    }

    auto [g, x, y] = extended_gcd(a, b);

    cout << "НОД(" << a << ", " << b << ") = " << g << endl;
    cout << "x = " << x << ", y = " << y << endl;
    cout << a << "*" << x << " + " << b << "*" << y << " = " << g << endl;

    return 0;
}

"""
Cryptographic utilities for Mental Poker
"""

import random
import math

def is_prime(n: int, k: int = 5) -> bool:
    """Проверяет, является ли число простым (тест Миллера-Рабина)"""
    if n == 2 or n == 3:
        return True
    if n <= 1 or n % 2 == 0:
        return False
    
    # Находим r и s
    s = 0
    r = n - 1
    while r & 1 == 0:
        s += 1
        r //= 2
    
    # Проводим k тестов
    for _ in range(k):
        a = random.randrange(2, n - 1)
        x = pow(a, r, n)
        if x != 1 and x != n - 1:
            j = 1
            while j < s and x != n - 1:
                x = pow(x, 2, n)
                if x == 1:
                    return False
                j += 1
            if x != n - 1:
                return False
    return True

def generate_prime(bits: int = 256) -> int:
    """Генерирует простое число заданной битности"""
    while True:
        num = random.getrandbits(bits)
        num |= (1 << bits - 1) | 1  # Устанавливаем старший бит и делаем нечетным
        if is_prime(num):
            return num

def extended_gcd(a: int, b: int) -> tuple:
    """Расширенный алгоритм Евклида"""
    if a == 0:
        return b, 0, 1
    gcd, x1, y1 = extended_gcd(b % a, a)
    x = y1 - (b // a) * x1
    y = x1
    return gcd, x, y

def mod_inverse(a: int, m: int) -> int:
    """Находит обратный элемент a по модулю m"""
    g, x, _ = extended_gcd(a, m)
    if g != 1:
        raise ValueError(f"Обратный элемент не существует для a={a}, m={m}")
    return x % m

def is_primitive_root(g: int, p: int) -> bool:
    """Проверяет, является ли g первообразным корнем по модулю p"""
    if pow(g, (p - 1) // 2, p) == 1:
        return False
    return True

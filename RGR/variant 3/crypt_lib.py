import random
import math
import hashlib

def fast_exp_mod(a, x, p):
    """
    Быстрое вычисление степени по модулю
    
    Args:
        a (int): Основание
        x (int): Показатель степени
        p (int): Модуль
    
    Returns:
        int: Результат вычисления a^x mod p
    """

    y = 1
    s = a % p
    while x > 0:
        if x % 2 == 1:
            y = (y * s) % p
        x = x // 2
        s = (s * s) % p
    return y



def fermat_primality_test(n, k=50):
    """
    Тест простоты Ферма
    
    Args:
        n (int): Число для проверки на простоту
        k (int): Количество тестов (по умолчанию 50)
    
    Returns:
        bool: True если число вероятно простое, False если составное
    """

    if n <= 1: return False
    if n <= 3: return True
    if n % 2 == 0: return False
    for _ in range(k):
        a = random.randint(2, n - 2)
        if fast_exp_mod(a, n - 1, n) != 1:
            return False
    return True



def extended_euclidean_algorithm(a, b):
    """
    Реализует обобщенный алгоритм Евклида.

    Находит наибольший общий делитель (НОД) двух чисел a и b, а также
    коэффициенты x и y, удовлетворяющие тождеству Безу:
    a*x + b*y = НОД(a, b).
    
    Args:
        a (int): Первое целое число.
        b (int): Второе целое число.
    
    Returns:
        tuple: Кортеж из трех целых чисел (gcd, x, y), где:
               gcd - наибольший общий делитель a и b.
               x, y - коэффициенты тождества Безу.
    """

    if a == 0:
        return b, 0, 1
    gcd, x1, y1 = extended_euclidean_algorithm(b % a, a)
    x = y1 - (b // a) * x1
    y = x1
    return gcd, x, y



def mod_inverse(n, modulus):
    """
    Находит модульный мультипликативный обратный элемент.

    Вычисляет такое число x, что (n * x) % modulus = 1.
    Функция использует обобщенный алгоритм Евклида. Обратный элемент
    существует только в том случае, если n и modulus взаимно просты.
    
    Args:
        n (int): Число, для которого ищется обратный элемент.
        modulus (int): Модуль, по которому вычисляется обратный элемент.
    
    Returns:
        int: Модульный обратный элемент для n по модулю modulus.
        
    Raises:
        Exception: Вызывается, если обратный элемент не существует
                   (т.е. НОД(n, modulus) != 1).
    """
    
    gcd, x, y = extended_euclidean_algorithm(n, modulus)
    if gcd != 1:
        raise Exception('Модульный обратный элемент не существует')
    return x % modulus



def generate_safe_prime(min_val, max_val):
    """
    Args:
        min_val (int): Минимальное значение для P.
        max_val (int): Максимальное значение для P.
        
    Returns:
        tuple: Кортеж (P, Q), где P - безопасное простое, Q - простое число Софи Жермен.
    """
    while True:
        q_candidate = random.randint(min_val // 2, max_val // 2)
        
        if fermat_primality_test(q_candidate):
            p_candidate = 2 * q_candidate + 1
            
            if fermat_primality_test(p_candidate):
                return p_candidate, q_candidate



def find_primitive_root(p, q):
    """
    Args:
        p (int): Безопасное простое число.
        q (int): Простое число Софи Жермен (q = (p-1)/2).
        
    Returns:
        int: Первообразный корень g.
    """
    for g in range(2, p):
        if fast_exp_mod(g, q, p) != 1:
            return g
    return None


    
def calculate_file_hash(filepath):
    """
    Вычисляет хэш SHA-256 файла
    """
    sha256_hash = hashlib.sha256()
    try:
        with open(filepath, "rb") as f:
            # Читаем файл по частям для экономии памяти
            for byte_block in iter(lambda: f.read(4096), b""):
                sha256_hash.update(byte_block)
        return sha256_hash.digest()
    except FileNotFoundError:
        return None


def file_hash_sha1(filepath):
    """
    Вычисляет хэш SHA-1 файла
    """
    sha256_hash = hashlib.sha1()
    try:
        with open(filepath, "rb") as f:
            # Читаем файл по частям для экономии памяти
            for byte_block in iter(lambda: f.read(4096), b""):
                sha256_hash.update(byte_block)
        return sha256_hash.digest()
    except FileNotFoundError:
        return None


def generate_prime_bits(bits):
    """Генерация простого числа заданной битности"""
    while True:
        num = random.randint(2**(bits-1), 2**bits - 1)
        if fermat_primality_test(num):
            return num
import random
import sys
import os
import math

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
    gcd, x, y = extended_euclidean_algorithm(n, modulus)
    if gcd != 1:
        raise Exception('Модульный обратный элемент не существует')
    return x % modulus

def generate_prime(bits=128):
    """Генерация простого числа """
    while True:
        n = random.getrandbits(bits)
        if n % 2 == 0: continue
        if is_prime(n):
            return n

def is_prime(n, k=5):
    if n <= 1: return False
    if n <= 3: return True
    if n % 2 == 0: return False
    for _ in range(k):
        a = random.randint(2, n - 2)
        if fast_exp_mod(a, n - 1, n) != 1:
            return False
    return True

def generate_rsa_keys(bits=256):
    """
    Генерирует параметры RSA.
    Возвращает (N, d, c), где:
    N - модуль
    d - открытый ключ
    c - закрытый ключ
    """
    p = generate_prime(bits)
    q = generate_prime(bits)
    while p == q:
        q = generate_prime(bits)
    
    N = p * q
    phi = (p - 1) * (q - 1)
    
    while True:
        d = random.randrange(3, phi)
        if math.gcd(d, phi) == 1:
            break
            
    c = mod_inverse(d, phi)
    return N, d, c



def read_graph_from_file(filename):
    """
    Чтение графа с жестким требованием нумерации вершин с 0 (0..N-1).
    """
    try:
        with open(filename, 'r') as f:
            lines = f.readlines()
            
        if not lines: return 0, [], []
        
        n, m = map(int, lines[0].strip().split())
        
        adj_matrix = [[0] * n for _ in range(n)]
        
        for i in range(1, m + 1):
            parts = list(map(int, lines[i].strip().split()))
            if len(parts) < 2: continue
            
            u, v = parts[0], parts[1]
            
            if u >= n or v >= n or u < 0 or v < 0:
                print(f"ОШИБКА в строке {i+1}: Вершина ({u} или {v}) выходит за диапазон [0, {n-1}].")
                return 0, [], []
                
            adj_matrix[u][v] = 1
            adj_matrix[v][u] = 1 
            
        cycle = list(map(int, lines[-1].split()))
        
        # Проверка цикла на границы
        if any(x >= n or x < 0 for x in cycle):
            print("ОШИБКА: Вершины в цикле выходят за диапазон [0, N-1].")
            return 0, [], []

        return n, adj_matrix, cycle
        
    except FileNotFoundError:
        print(f"Файл {filename} не найден.")
        return 0, [], []
    except ValueError:
        print("Ошибка парсинга файла (некорректные числа).")
        return 0, [], []



class Verifier:
    def __init__(self):
        self.F = None # Зашифрованная матрица
        self.N = None
        self.d = None # Публичный ключ
        self.G_n = 0  # Количество вершин исходного графа
    
    def receive_commitment(self, F, N, d, n):
        """Пункт 5 (начало): Получение зашифрованного графа."""
        self.F = F
        self.N = N
        self.d = d
        self.G_n = n
        print("[Verifier] Получена зашифрованная матрица F.")

    def send_challenge(self):
        """Пункт 5: Задает один из вопросов (0 или 1)."""
        # 0 -> Вопрос 1: Каков гамильтонов цикл?
        # 1 -> Вопрос 2: Действительно ли H изоморфен G?
        challenge = random.choice([1, 2])
        print(f"[Verifier] Бросаю монету... Вопрос № {challenge}")
        return challenge

    def verify_response(self, challenge, response, original_G_matrix):
        """Пункт 7: Проверка ответа."""
        if challenge == 1:
            # Ответ: список ребер (u, v, значение_H_prime)
            cycle_edges_data = response
            
            # 1. Проверяем, образуют ли ребра цикл длины n
            if len(cycle_edges_data) != self.G_n:
                print("[Verifier] ОШИБКА: Длина цикла не совпадает с количеством вершин.")
                return False
            
            # Проверка связности цикла
            visited = set()
            path = []
            for u, v, val in cycle_edges_data:
                path.append((u, v))
                visited.add(u)
                visited.add(v)
            
            if len(visited) != self.G_n:
                 print("[Verifier] ОШИБКА: Не все вершины посещены.")
                 return False
                 
            # 2. Проверяем расшифровку и наличие единиц
            for (i, j, h_prime_val) in cycle_edges_data:
                # Повторное шифрование
                encrypted = pow(h_prime_val, self.d, self.N)
                if encrypted != self.F[i][j]:
                    print(f"[Verifier] ОШИБКА: Неверная расшифровка ребра ({i}, {j}).")
                    return False
                
                # Проверка, что ребро существует (последний бит == 1)
                # h_prime_val это r || bit. Значит строка должна кончаться на '1'
                s_val = str(h_prime_val)
                if not s_val.endswith('1'):
                    print(f"[Verifier] ОШИБКА: Ребро ({i}, {j}) в графе H равно 0 (отсутствует).")
                    return False
            
            print("[Verifier] УСПЕХ: Гамильтонов цикл подтвержден.")
            return True

        elif challenge == 2:
            # Ответ: (H_prime_matrix, permutation)
            H_prime, perm = response
            n = self.G_n
            
            # 1. Проверяем корректность расшифровки всей матрицы
            for r in range(n):
                for c in range(n):
                    encrypted = pow(H_prime[r][c], self.d, self.N)
                    if encrypted != self.F[r][c]:
                         print(f"[Verifier] ОШИБКА: Неверная расшифровка ячейки ({r}, {c}).")
                         return False
            
            # 2. Восстанавливаем H из H_prime (отбрасываем random)
            H = [[0]*n for _ in range(n)]
            for r in range(n):
                for c in range(n):
                    val_str = str(H_prime[r][c])
                    bit = int(val_str[-1]) # Последний символ
                    H[r][c] = bit
            
            # 3. Проверяем изоморфизм
            # H должен быть получен из G перестановкой perm
            # H[new_u][new_v] == G[u][v], где new_u = perm[u]
            
            # Создадим граф H_from_G на основе G и perm
            # perm[old_index] -> new_index
            for i in range(n):
                for j in range(n):
                    u_new = perm[i]
                    v_new = perm[j]
                    if H[u_new][v_new] != original_G_matrix[i][j]:
                        print(f"[Verifier] ОШИБКА: Граф H не изоморфен G. Несовпадение в ({i}->{u_new}, {j}->{v_new}).")
                        return False
            
            print("[Verifier] УСПЕХ: Изоморфизм графа доказан.")
            return True



class Prover:
    def __init__(self, n, G_matrix, G_cycle):
        self.n = n
        self.G = G_matrix
        self.cycle = G_cycle
        
        self.N = 0
        self.d = 0
        self.c = 0
        self.perm = []     # Перестановка pi
        self.H = []        # Изоморфный граф
        self.H_prime = []  # Матрица с r || bit
        self.F = []        # Зашифрованная матрица

    def generate_keys(self):
        """Генерация ключей RSA."""
        self.N, self.d, self.c = generate_rsa_keys(bits=512)

    def build_isomorphic(self):
        """Построить H изоморфный G."""
        indices = list(range(self.n))
        random.shuffle(indices)
        self.perm = indices # Это отображение i -> pi(i)
        
        # Строим H
        self.H = [[0] * self.n for _ in range(self.n)]
        for i in range(self.n):
            for j in range(self.n):
                new_i = self.perm[i]
                new_j = self.perm[j]
                self.H[new_i][new_j] = self.G[i][j]

    def encode_and_encrypt(self):
        """Пункт 3 и 4: Кодирование и Шифрование."""
        self.H_prime = [[0] * self.n for _ in range(self.n)]
        self.F = [[0] * self.n for _ in range(self.n)]
        
        for i in range(self.n):
            for j in range(self.n):
                # 3) H'[ij] = r[ij] || H[ij]
                # Генерируем случайное число r
                r = random.getrandbits(64)
                bit = self.H[i][j]
                
                # Конкатенация: превращаем в строку, склеиваем, обратно в int
                # Это гарантирует, что последний разряд - это бит графа
                encoded_val = int(str(r) + str(bit))
                self.H_prime[i][j] = encoded_val
                
                # 4) F[ij] = H'[ij]^d mod N
                # Используем d как экспоненту шифрования (согласно заданию)
                encrypted_val = pow(encoded_val, self.d, self.N)
                self.F[i][j] = encrypted_val
        
        return self.F, self.N, self.d

    def solve_challenge(self, challenge):
        """Пункт 6: Ответ на вопрос проверяющего."""
        if challenge == 1:
            # 1. Расшифровывает в F ребра, образующие гамильтонов цикл.
            # Нам нужно найти координаты цикла в графе H
            
            # Переводим цикл из координат G в координаты H
            h_cycle_nodes = [self.perm[node] for node in self.cycle]
            
            response_data = []
            
            # Проходим по циклу. i соединяется с i+1
            for k in range(len(h_cycle_nodes)):
                u = h_cycle_nodes[k]
                v = h_cycle_nodes[(k + 1) % len(h_cycle_nodes)]
                
                # Достаем "расшифрованное" значение (исходное H_prime)
                val = self.H_prime[u][v]
                response_data.append((u, v, val))
                
            print(f"[Prover] Раскрываю ребра гамильтонова цикла в H.")
            return response_data

        elif challenge == 2:
            # 2. Расшифровывает F полностью (передает H') и перестановки
            print(f"[Prover] Раскрываю всю матрицу H' и перестановку.")
            return (self.H_prime, self.perm)



def main():
    filename = "graph.txt"
    if not os.path.exists(filename):
        print ('Файл не найден')
        return -1
    
    n, G, cycle = read_graph_from_file(filename)
    if n == 0: return

    print(f"Граф загружен: {n} вершин.")
    print(f"Гамильтонов цикл (вершины): {cycle}")
    
    rounds = 5
    
    verifier = Verifier()
    prover = Prover(n, G, cycle)
    
    print("\n--- Начало ZKP протокола ---")
    
    for t in range(1, rounds + 1):
        print(f"\n>>> Раунд {t} <<<")
        
        prover.generate_keys()
        
        prover.build_isomorphic()
        
        F, N, d = prover.encode_and_encrypt()
        
        verifier.receive_commitment(F, N, d, n)
        chal = verifier.send_challenge()
        
        resp = prover.solve_challenge(chal)
        
        is_valid = verifier.verify_response(chal, resp, G)
        
        if not is_valid:
            print("Доказательство ОТКЛОНЕНО.")
            break
        else:
            print("Раунд пройден успешно.")
            
    print("\n--- Протокол завершен ---")

if __name__ == "__main__":
    main()
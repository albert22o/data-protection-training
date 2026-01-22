import socket
import json
import random
import hashlib

HOST = '127.0.0.1'
PORT = 65432

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



def derive_secret(password, N):
    # s генерируется из хэша пароля
    h = hashlib.sha256(password.encode()).hexdigest()
    s = int(h, 16) % N
    if s == 0: s = 1
    return s

def run_client():
    print("--- Клиент Фиата-Шамира ---")
    action = input("1 - Регистрация\n2 - Вход\nВаш выбор: ").strip()
    login = input("Логин: ").strip()
    password = input("Пароль: ").strip()

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((HOST, PORT))

        # 1. Получаем N
        data = sock.recv(1024).decode('utf-8')
        params = json.loads(data)
        N = params['N']
        rounds = params['rounds']
        
        # Вычисляем s и v
        s = derive_secret(password, N)
        # v = s^2 mod N (используем библиотеку)
        v = fast_exp_mod(s, 2, N)

        if action == '1':
            # === РЕГИСТРАЦИЯ ===
            req = {'mode': 'REGISTER', 'login': login, 'v': v}
            sock.sendall(json.dumps(req).encode('utf-8'))
            print(f"Ответ сервера: {sock.recv(1024).decode('utf-8')}")

        elif action == '2':
            # === ВХОД ===
            req = {'mode': 'LOGIN', 'login': login}
            sock.sendall(json.dumps(req).encode('utf-8'))

            print("Начинается проверка...")
            
            for i in range(rounds):
                # Шаг 1: x = r^2 mod N
                r = random.randint(1, N - 1)
                x = fast_exp_mod(r, 2, N)
                
                sock.sendall(json.dumps({'x': x}).encode('utf-8'))

                # Шаг 2: Получаем e
                server_msg = sock.recv(1024).decode('utf-8')
                
                # Обработка случая, если сервер вернул ошибку раньше времени (на всякий случай)
                if "USER_NOT_FOUND" in server_msg:
                    print("Ошибка: Пользователь не найден.")
                    return
                
                try:
                    server_json = json.loads(server_msg)
                    e = server_json['e']
                except json.JSONDecodeError:
                    print(f"Ошибка протокола: получено '{server_msg}' вместо JSON")
                    return

                # Шаг 3: Вычисляем y
                if e == 0:
                    y = r
                else:
                    # y = r * s mod N
                    y = (r * s) % N
                
                sock.sendall(json.dumps({'y': y}).encode('utf-8'))

            # Финальный ответ
            result = sock.recv(1024).decode('utf-8')
            if result == "AUTH_SUCCESS":
                print(">>> ДОСТУП РАЗРЕШЕН")
            else:
                print(">>> ОТКАЗ В ДОСТУПЕ (Неверный пароль)")

    except ConnectionRefusedError:
        print("Не удалось подключиться к серверу.")
    except Exception as e:
        print(f"Ошибка: {e}")
    finally:
        sock.close()

if __name__ == "__main__":
    run_client()
import socket
import json
import random
import os
import threading
import crypt_lib

# Конфигурация
HOST = '127.0.0.1'
PORT = 65432
DB_FILE = 'users_db.json'
ROUNDS = 32
PRIME_BITS = 128

# Генерация N с использованием crypt_lib
print(">>> Генерация параметров системы (может занять время)...")
p = crypt_lib.generate_prime_bits(PRIME_BITS)
q = crypt_lib.generate_prime_bits(PRIME_BITS)
# Убедимся, что p != q
while p == q:
    q = crypt_lib.generate_prime_bits(PRIME_BITS)

N = p * q
print(f">>> Параметры сгенерированы. N имеет длину {N.bit_length()} бит.")

def load_users():
    if not os.path.exists(DB_FILE):
        return {}
    with open(DB_FILE, 'r') as f:
        try:
            return json.load(f)
        except json.JSONDecodeError:
            return {}

def save_user(login, public_key):
    users = load_users()
    users[login] = public_key
    with open(DB_FILE, 'w') as f:
        json.dump(users, f)
    print(f"[DB] Пользователь {login} сохранен.")

def handle_client(conn, addr):
    print(f"[NEW CONNECTION] {addr} connected.")
    
    try:
        # 1. Отправляем N клиенту
        initial_params = json.dumps({'N': N, 'rounds': ROUNDS})
        conn.sendall(initial_params.encode('utf-8'))

        # 2. Ждем команду
        data = conn.recv(1024).decode('utf-8')
        request = json.loads(data)
        
        mode = request.get('mode')
        login = request.get('login')

        if mode == 'REGISTER':
            v = request.get('v')
            save_user(login, v)
            conn.sendall(b"REGISTRATION_SUCCESS")
            
        elif mode == 'LOGIN':
            users = load_users()
            if login not in users:
                conn.sendall(b"USER_NOT_FOUND")
                return

            v = users[login]
            print(f"[AUTH] Проверка {login}...")
            
            is_authenticated = True
            
            # Протокол Фиата-Шамира
            for i in range(ROUNDS):
                try:
                    # Шаг 1: Получаем x
                    msg = conn.recv(1024).decode('utf-8')
                    if not msg: break
                    client_data = json.loads(msg)
                    x = client_data['x']
                    
                    # Шаг 2: Отправляем e (0 или 1)
                    e = random.choice([0, 1])
                    conn.sendall(json.dumps({'e': e}).encode('utf-8'))
                    
                    # Шаг 3: Получаем y
                    msg = conn.recv(1024).decode('utf-8')
                    client_data = json.loads(msg)
                    y = client_data['y']
                    
                    # Шаг 4: Проверка y^2 = x * v^e mod N
                    # Используем библиотечную функцию fast_exp_mod
                    left = crypt_lib.fast_exp_mod(y, 2, N)
                    
                    # right = (x * v^e) % N
                    v_pow_e = crypt_lib.fast_exp_mod(v, e, N)
                    right = (x * v_pow_e) % N
                    
                    if left != right:
                        print(f"[AUTH] {login}: Ошибка в раунде {i+1}")
                        is_authenticated = False
                        # ВАЖНО: Мы НЕ делаем break, чтобы не сбить клиента
                        # Мы просто помечаем флаг как False и продолжаем "играть" до конца
                except Exception as e:
                    print(f"[ERROR in loop] {e}")
                    is_authenticated = False
                    break
            
            # Результат отправляем только после всех раундов
            if is_authenticated:
                conn.sendall(b"AUTH_SUCCESS")
                print(f"[AUTH] {login}: УСПЕХ")
            else:
                conn.sendall(b"AUTH_FAILED")
                print(f"[AUTH] {login}: ОТКАЗ")

    except Exception as exc:
        print(f"[ERROR] {exc}")
    finally:
        conn.close()

def start_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((HOST, PORT))
    server.listen()
    print(f"[LISTENING] Сервер на {HOST}:{PORT}")
    
    while True:
        conn, addr = server.accept()
        thread = threading.Thread(target=handle_client, args=(conn, addr))
        thread.start()

if __name__ == "__main__":
    start_server()
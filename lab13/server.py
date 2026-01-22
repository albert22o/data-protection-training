import random
import math
import hashlib
import crypt_lib as cl

def rsa_generate_params(min_p=255, max_p=65535):
    """
    Генерирует полный набор параметров для протокола RSA.
    (Код предоставлен в условии)
    """
    while True:
        p_candidate = random.randint(min_p, max_p)
        if cl.fermat_primality_test(p_candidate):
            p = p_candidate
            break

    while True:
        q_candidate = random.randint(min_p, max_p)
        if cl.fermat_primality_test(q_candidate) and q_candidate != p:
            q = q_candidate
            break
        
    n_big = p*q

    phi = (p-1)*(q-1)
    while True:
        d_candidate = random.randint(2, phi - 1)
        if math.gcd(d_candidate, phi) == 1:
            public_key = d_candidate
            break

    private_key = cl.mod_inverse(public_key, phi)

    return n_big, public_key, private_key

class VotingServer:
    def __init__(self):
        self.N, self.D, self.C = rsa_generate_params(min_p=1000, max_p=50000)
        
        self.voters_who_received_ballots = set()
        
        self.ballot_box = [] 

    def get_public_key(self):
        """Возвращает открытые данные (N, D)."""
        return self.N, self.D

    def sign_blind_ballot(self, voter_id, blind_hash):
        """
        Этап 1: Сервер подписывает зашифрованный (слепой) хеш бюллетеня.
        Проверяет, имеет ли право voter_id голосовать.
        """
        if voter_id in self.voters_who_received_ballots:
            raise Exception(f"Пользователь {voter_id} уже получил бюллетень!")
        
        self.voters_who_received_ballots.add(voter_id)
        
        s_prime = pow(blind_hash, self.C, self.N)
        
        return s_prime

    def receive_filled_ballot(self, n, s):
        """
        Этап 2: Анонимный канал. Сервер получает пару <n, s>.
        Проверяет подпись и учитывает голос.
        """
        h_bytes = hashlib.sha3_256(str(n).encode('utf-8')).digest()
        h = int.from_bytes(h_bytes, byteorder='big')
        
        h = h % self.N

        decrypted_signature = pow(s, self.D, self.N)
        
        if h == decrypted_signature:
            print(f"[Server] Бюллетень валиден. Голос принят.")
            self.ballot_box.append(n)
            return True
        else:
            print(f"[Server] ОШИБКА: Подпись недействительна!")
            return False

    def show_results(self):
        """Подсчет и вывод результатов (декодирование n)."""
        print("\n--- Результаты голосования ---")
        results = {"да": 0, "нет": 0, "воздержался": 0}
        
        for n in self.ballot_box:

            v = n & 0b11 # Берем последние 2 бита
            
            if v == 1: results["да"] += 1
            elif v == 2: results["нет"] += 1
            elif v == 3: results["воздержался"] += 1
            
        print(results)
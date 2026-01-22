import random
import hashlib
import crypt_lib as cl
import server 

class Voter:
    def __init__(self, name, voting_server):
        self.name = name
        self.server = voting_server

    def vote(self, choice):
        """
        Основной метод голосования.
        choice: "да", "нет", "воздержался"
        """

        mapping = {"да": 1, "нет": 2, "воздержался": 3}
        if choice not in mapping:
            print("Некорректный выбор")
            return
        v = mapping[choice]

        N, D = self.server.get_public_key()

        rnd = random.getrandbits(32) 
        n = (rnd << 2) | v
        
        h_bytes = hashlib.sha3_256(str(n).encode('utf-8')).digest()
        h = int.from_bytes(h_bytes, byteorder='big')
        h = h % N

        while True:
            r = random.randint(2, N - 1)
            if cl.mod_inverse(r, N) != 0:
                break
        
        h_prime = (h * pow(r, D, N)) % N

        print(f"[{self.name}] Отправляет слепой хеш на подпись...")
        
        try:
            s_prime = self.server.sign_blind_ballot(self.name, h_prime)
        except Exception as e:
            print(f"[{self.name}] Ошибка сервера: {e}")
            return

        r_inv = cl.mod_inverse(r, N)
        s = (s_prime * r_inv) % N

        print(f"[{self.name}] Подпись получена и расшифрована. Отправка бюллетеня...")

        self.server.receive_filled_ballot(n, s)


if __name__ == "__main__":
    srv = server.VotingServer()
    print("Сервер запущен. Параметры сгенерированы.")

    alice = Voter("Alice", srv)
    bob = Voter("Bob", srv)
    leha = Voter("Leha", srv)
    lera = Voter("Lera", srv)

    alice.vote("да")
    bob.vote("нет")
    leha.vote("да")
    lera.vote("воздержался")
    
    alice.vote("воздержался")

    srv.show_results()
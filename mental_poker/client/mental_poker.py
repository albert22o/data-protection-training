import random
import math
from shared.crypto_utils import generate_prime, mod_inverse, is_primitive_root

class MentalPoker:
    def __init__(self, num_players: int, p: int = None):
        self.p = p or generate_prime(256)  # 256-битное простое число
        self.num_players = num_players
        self.players = [Player(i, self.p) for i in range(num_players)]
        self.deck = self._generate_deck()
        self.encrypted_deck = None
        
    def _generate_deck(self) -> list:
        """Создает колоду из 52 уникальных карт"""
        cards = []
        used_numbers = set()
        
        # Генерируем 52 уникальных числа в диапазоне [2, p-1]
        while len(cards) < 52:
            card = random.randint(2, self.p - 1)
            if card not in used_numbers:
                cards.append(card)
                used_numbers.add(card)
        
        # Сопоставляем числа с картами
        self.card_mapping = {}
        suits = ['♠', '♥', '♦', '♣']
        ranks = ['2', '3', '4', '5', '6', '7', '8', '9', '10', 'J', 'Q', 'K', 'A']
        
        for i, card_num in enumerate(cards):
            suit = suits[i // 13]
            rank = ranks[i % 13]
            self.card_mapping[card_num] = f"{rank}{suit}"
            self.card_mapping[f"{rank}{suit}"] = card_num  # Обратное отображение
        
        return cards
    
    def get_card_name(self, card_value: int) -> str:
        """Возвращает название карты по ее числовому значению"""
        return self.card_mapping.get(card_value, str(card_value))
    
    def get_card_value(self, card_name: str) -> int:
        """Возвращает числовое значение карты по ее названию"""
        return self.card_mapping.get(card_name)

    def encrypt_deck(self) -> list:
        """Упрощенная версия для тестирования"""
        # Вместо реального шифрования возвращаем простой список
        print("DEBUG: Using simplified deck encryption for testing")
        return [i for i in range(52)]  # Просто возвращаем числа 0-51

    def network_encrypt_deck(self):
        """Сетевое шифрование колоды - упрощенная версия"""
        print("DEBUG: Starting simplified deck encryption")
        encrypted_deck = self.encrypt_deck()

        # Отправка зашифрованной колоды на сервер
        self.client.send_message({
            'type': 'encryption_done',
            'player_id': self.client.player_id,
            'encrypted_deck': encrypted_deck
        })

    def decrypt_card(self, encrypted_card: int, player_index: int) -> int:
        """Расшифровывает карту для конкретного игрока"""
        card = encrypted_card
        # Все игроки кроме целевого частично расшифровывают карту
        for i, player in enumerate(self.players):
            if i != player_index:
                card = player.partial_decrypt(card)
        
        # Целевой игрок полностью расшифровывает карту
        card = self.players[player_index].decrypt(card)
        return card
    
    def deal_private_cards(self, num_cards_per_player: int = 2) -> dict:
        """Раздает приватные карты игрокам"""
        if not self.encrypted_deck:
            raise ValueError("Deck not encrypted yet")
        
        hands = {}
        used_indices = set()
        
        for player_index in range(self.num_players):
            hand = []
            for _ in range(num_cards_per_player):
                # Выбираем случайную незанятую карту
                available = [i for i in range(len(self.encrypted_deck)) if i not in used_indices]
                if not available:
                    raise ValueError("Not enough cards in deck")
                
                card_index = random.choice(available)
                used_indices.add(card_index)
                
                # Расшифровываем карту для игрока
                encrypted_card = self.encrypted_deck[card_index]
                decrypted_card = self.decrypt_card(encrypted_card, player_index)
                hand.append(decrypted_card)
            
            hands[player_index] = hand
        
        return hands
    
    def deal_community_cards(self, num_cards: int = 5) -> list:
        """Раздает общие карты на стол"""
        if not self.encrypted_deck:
            raise ValueError("Deck not encrypted yet")
        
        community_cards = []
        used_indices = set()
        
        # Находим уже использованные индексы из приватных карт
        # В реальной реализации нужно отслеживать использованные карты
        
        for _ in range(num_cards):
            available = [i for i in range(len(self.encrypted_deck)) if i not in used_indices]
            if not available:
                raise ValueError("Not enough cards in deck")
            
            card_index = random.choice(available)
            used_indices.add(card_index)
            
            # Общие карты расшифровываются публично
            encrypted_card = self.encrypted_deck[card_index]
            decrypted_card = encrypted_card
            
            # Все игроки по очереди расшифровывают
            for player in self.players:
                decrypted_card = player.partial_decrypt(decrypted_card)
            
            community_cards.append(decrypted_card)
        
        return community_cards

class Player:
    def __init__(self, player_id: int, p: int):
        self.id = player_id
        self.p = p
        self._generate_keys()
    
    def _generate_keys(self):
        """Генерирует пару ключей для шифрования/расшифрования"""
        # Выбираем случайный секретный ключ
        self.secret_key = random.randint(2, self.p - 2)
        
        # Вычисляем открытый ключ
        # В упрощенной версии используем простую схему
        self.public_key = pow(2, self.secret_key, self.p)
    
    def encrypt(self, card: int) -> int:
        """Шифрует карту используя открытый ключ игрока"""
        return pow(card, self.secret_key, self.p)
    
    def partial_decrypt(self, encrypted_card: int) -> int:
        """Частично расшифровывает карту используя секретный ключ"""
        # В реальной реализации здесь должна быть более сложная логика
        # Для демонстрации используем обратную операцию
        try:
            return pow(encrypted_card, mod_inverse(self.secret_key, self.p - 1), self.p)
        except:
            return encrypted_card
    
    def decrypt(self, partially_decrypted_card: int) -> int:
        """Полностью расшифровывает карту"""
        # В этой упрощенной реализации partial_decrypt уже делает полное расшифрование
        # для целевого игрока
        return partially_decrypted_card

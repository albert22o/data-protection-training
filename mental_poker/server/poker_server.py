import socket
import threading
import json
import logging
import uuid
from typing import Dict, List, Any
from server.game_session import GameSession

class PokerServer:
    def __init__(self, host='localhost', port=8888):
        self.host = host
        self.port = port
        self.sessions: Dict[str, GameSession] = {}
        self.players: Dict[str, Any] = {}  # player_id -> {socket, name, session_id}
        self.lock = threading.Lock()
        self.logger = logging.getLogger('PokerServer')
        
    def start(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind((self.host, self.port))
        self.socket.listen(6)  # До 6 игроков
        self.logger.info(f"Server started on {self.host}:{self.port}")
        
        while True:
            client_socket, address = self.socket.accept()
            self.logger.info(f"New connection from {address}")
            threading.Thread(target=self.handle_client, args=(client_socket, address), daemon=True).start()
    
    def handle_client(self, client_socket, address):
        player_id = None
        try:
            while True:
                data = client_socket.recv(4096)
                if not data:
                    break
                    
                try:
                    message = json.loads(data.decode('utf-8'))
                    response = self.process_message(client_socket, message)
                    if response:
                        client_socket.send(json.dumps(response).encode('utf-8'))
                except json.JSONDecodeError as e:
                    self.logger.error(f"JSON decode error from {address}: {e}")
                    error_msg = {'type': 'error', 'message': 'Invalid JSON'}
                    client_socket.send(json.dumps(error_msg).encode('utf-8'))
                
        except Exception as e:
            self.logger.error(f"Client handling error: {e}")
        finally:
            if player_id and player_id in self.players:
                self.handle_player_disconnect(player_id)
            client_socket.close()
            self.logger.info(f"Connection closed from {address}")

    def process_message(self, client_socket, message):
        msg_type = message.get('type')
        self.logger.debug(f"Processing message type: {msg_type}")

        try:
            if msg_type == 'register':
                return self.register_player(client_socket, message)
            elif msg_type == 'create_session':
                response = self.create_session(message)
                # После создания сессии обновляем список для всех
                self.broadcast_session_list()
                return response
            elif msg_type == 'join_session':
                return self.join_session(message)
            elif msg_type == 'get_sessions':
                return {'type': 'session_list', 'sessions': self.get_available_sessions()}
            elif msg_type == 'start_game':
                return self.start_game(message)
            elif msg_type == 'encryption_done':
                return self.handle_encryption_done(message)
            elif msg_type == 'card_selection':
                return self.handle_card_selection(message)
            elif msg_type == 'partial_decrypt':
                return self.handle_partial_decrypt(message)
            elif msg_type == 'bet_action':
                return self.handle_bet_action(message)
            elif msg_type == 'chat_message':
                return self.handle_chat_message(message)
            else:
                return {'type': 'error', 'message': f'Unknown message type: {msg_type}'}
        except Exception as e:
            self.logger.error(f"Error processing message: {e}")
            return {'type': 'error', 'message': f'Internal server error: {str(e)}'}
    
    def register_player(self, client_socket, message):
        player_name = message.get('player_name', 'Unknown')
        player_id = str(uuid.uuid4())[:8]
        
        with self.lock:
            self.players[player_id] = {
                'socket': client_socket,
                'name': player_name,
                'session_id': None,
                'ready': False
            }
        
        self.logger.info(f"Player registered: {player_name} ({player_id})")
        return {
            'type': 'register_response',
            'player_id': player_id,
            'status': 'success'
        }
    
    def create_session(self, message):
        player_id = message.get('player_id')
        session_name = message.get('session_name', 'Poker Game')
        
        if player_id not in self.players:
            return {'type': 'error', 'message': 'Player not registered'}
        
        session_id = str(uuid.uuid4())[:8]
        session = GameSession(session_id, session_name)
        
        with self.lock:
            self.sessions[session_id] = session
            # Автоматически присоединяем создателя к сессии
            self.join_session_internal(player_id, session_id)
        
        self.logger.info(f"Session created: {session_name} ({session_id}) by {player_id}")
        return {
            'type': 'session_created',
            'session_id': session_id,
            'session_name': session_name
        }
    
    def join_session(self, message):
        player_id = message.get('player_id')
        session_id = message.get('session_id')
        
        if player_id not in self.players:
            return {'type': 'error', 'message': 'Player not registered'}
        
        if session_id not in self.sessions:
            return {'type': 'error', 'message': 'Session not found'}
        
        return self.join_session_internal(player_id, session_id)
    
    def join_session_internal(self, player_id, session_id):
        session = self.sessions[session_id]
        player_info = self.players[player_id]
        
        if session.add_player(player_id, player_info['name']):
            player_info['session_id'] = session_id
            
            # Уведомляем всех игроков в сессии
            self.broadcast_to_session(session_id, {
                'type': 'player_joined',
                'player_id': player_id,
                'player_name': player_info['name'],
                'players': session.get_players_info()
            })
            
            self.logger.info(f"Player {player_id} joined session {session_id}")
            return {
                'type': 'session_joined',
                'session_id': session_id,
                'session_name': session.name,
                'players': session.get_players_info()
            }
        else:
            return {'type': 'error', 'message': 'Session is full'}
    
    def start_game(self, message):
        player_id = message.get('player_id')
        session_id = self.players[player_id].get('session_id')
        
        if not session_id or session_id not in self.sessions:
            return {'type': 'error', 'message': 'Not in a session'}
        
        session = self.sessions[session_id]
        if session.start_game():
            self.broadcast_to_session(session_id, {
                'type': 'game_started',
                'message': 'Game is starting!'
            })
            return {'type': 'game_start_success'}
        else:
            return {'type': 'error', 'message': 'Not enough players to start (min 2)'}

    def handle_encryption_done(self, message):
        player_id = message.get('player_id')
        session_id = self.players[player_id].get('session_id')

        if session_id and session_id in self.sessions:
            session = self.sessions[session_id]

            # Для демонстрации просто отмечаем игрока как готового
            if session.set_player_ready(player_id, []):
                # Все игроки готовы, начинаем раздачу карт
                self.start_card_dealing(session_id)

            return {'type': 'encryption_acknowledged'}

        return {'type': 'error', 'message': 'Session not found'}
    
    def handle_card_selection(self, message):
        # Маршрутизация сообщений выбора карт между игроками
        target_player = message.get('target_player')
        if target_player in self.players:
            self.send_to_player(target_player, message)
        
        return {'type': 'routing_acknowledged'}
    
    def handle_partial_decrypt(self, message):
        # Маршрутизация сообщений частичного расшифрования
        target_player = message.get('target_player')
        if target_player in self.players:
            self.send_to_player(target_player, message)
        
        return {'type': 'routing_acknowledged'}
    
    def handle_bet_action(self, message):
        player_id = message.get('player_id')
        session_id = self.players[player_id].get('session_id')
        
        if session_id and session_id in self.sessions:
            session = self.sessions[session_id]
            # Здесь должна быть логика обработки ставок
            # Пока просто пересылаем всем в сессии
            self.broadcast_to_session(session_id, message)
        
        return {'type': 'bet_acknowledged'}

    def handle_chat_message(self, message):
        player_id = message.get('player_id')
        if player_id not in self.players:
            return {'type': 'error', 'message': 'Player not found'}

        session_id = self.players[player_id].get('session_id')

        if session_id and session_id in self.sessions:
            player_name = self.players[player_id]['name']
            chat_msg = message.get('message', '')

            # Отправляем сообщение ВСЕМ в сессии, включая отправителя
            self.broadcast_to_session(session_id, {
                'type': 'chat_message',
                'player_name': player_name,
                'message': chat_msg,
                'timestamp': message.get('timestamp')
            })

        return {'type': 'chat_acknowledged'}

    def start_card_dealing(self, session_id):
        """Начинает процесс раздачи карт"""
        session = self.sessions[session_id]

        # Симуляция раздачи карт
        card_dealing_result = {
            'type': 'card_dealing_complete',
            'player_cards': {},
            'community_cards': ['A♠', 'K♥', 'Q♦', 'J♣', '10♠']  # Пример
        }

        # Симулируем карты для каждого игрока
        for i, player_id in enumerate(session.players):
            card_dealing_result['player_cards'][player_id] = [f'{i + 2}♥', f'{i + 3}♠']  # Пример

        # Отправляем результаты всем игрокам
        self.broadcast_to_session(session_id, card_dealing_result)

        # Обновляем состояние игры
        session.game_state['phase'] = 'preflop'
        self.broadcast_to_session(session_id, {
            'type': 'game_state_update',
            'game_state': session.get_game_state()
        })
    
    def broadcast_to_session(self, session_id, message):
        if session_id in self.sessions:
            session = self.sessions[session_id]
            for player_id in session.players:
                if player_id in self.players:
                    self.send_to_player(player_id, message)
    
    def send_to_player(self, player_id, message):
        if player_id in self.players:
            try:
                socket = self.players[player_id]['socket']
                socket.send(json.dumps(message).encode('utf-8'))
            except Exception as e:
                self.logger.error(f"Error sending to player {player_id}: {e}")
    
    def handle_player_disconnect(self, player_id):
        if player_id in self.players:
            player_info = self.players[player_id]
            session_id = player_info.get('session_id')
            
            if session_id and session_id in self.sessions:
                session = self.sessions[session_id]
                session.remove_player(player_id)
                
                # Уведомляем остальных игроков
                self.broadcast_to_session(session_id, {
                    'type': 'player_left',
                    'player_id': player_id,
                    'player_name': player_info['name'],
                    'players': session.get_players_info()
                })
            
            del self.players[player_id]
            self.logger.info(f"Player disconnected: {player_id}")

    def get_available_sessions(self):
        """Возвращает список доступных сессий"""
        return [{
            'session_id': session_id,
            'name': session.name,
            'players_count': len(session.players),
            'max_players': session.max_players
        } for session_id, session in self.sessions.items()
           if len(session.players) < session.max_players]

    def broadcast_session_list(self):
        """Рассылает обновленный список сессий всем подключенным игрокам"""
        sessions_list = self.get_available_sessions()
        for player_id, player_info in self.players.items():
            if player_info['socket']:
                self.send_to_player(player_id, {
                    'type': 'session_list_update',
                    'sessions': sessions_list
                })
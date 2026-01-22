import socket
import threading
import json
import logging
from typing import Callable, Any

class PokerClient:
    def __init__(self):
        self.socket = None
        self.connected = False
        self.player_id = None
        self.session_id = None
        self.message_handlers = {}
        self.logger = logging.getLogger('PokerClient')
        
    def connect(self, host: str, port: int, player_name: str) -> bool:
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((host, port))
            self.connected = True
            
            # Запускаем поток для приема сообщений
            threading.Thread(target=self._receive_loop, daemon=True).start()
            
            # Регистрируем игрока
            self.send_message('register', player_name=player_name)
            return True
            
        except Exception as e:
            self.logger.error(f"Connection error: {e}")
            return False
    
    def disconnect(self):
        self.connected = False
        if self.socket:
            self.socket.close()
            self.socket = None
    
    def send_message(self, msg_type: str, **kwargs):
        if not self.connected or not self.socket:
            return
        
        message = {'type': msg_type, **kwargs}
        if self.player_id:
            message['player_id'] = self.player_id
        if self.session_id:
            message['session_id'] = self.session_id
            
        try:
            self.socket.send(json.dumps(message).encode('utf-8'))
        except Exception as e:
            self.logger.error(f"Send error: {e}")
            self.connected = False
    
    def _receive_loop(self):
        while self.connected:
            try:
                data = self.socket.recv(4096)
                if not data:
                    break
                
                message = json.loads(data.decode('utf-8'))
                self._handle_message(message)
                
            except json.JSONDecodeError as e:
                self.logger.error(f"JSON decode error: {e}")
            except Exception as e:
                self.logger.error(f"Receive error: {e}")
                break
        
        self.connected = False
        if self.socket:
            self.socket.close()
            self.socket = None
        
        # Уведомляем о разрыве соединения
        self._call_handler('disconnected', {})
    
    def _handle_message(self, message: dict):
        msg_type = message.get('type')
        self.logger.debug(f"Received message: {msg_type}")
        
        # Обрабатываем специальные сообщения
        if msg_type == 'register_response':
            if message.get('status') == 'success':
                self.player_id = message.get('player_id')
        
        # Вызываем зарегистрированный обработчик
        self._call_handler(msg_type, message)
    
    def register_handler(self, msg_type: str, handler: Callable[[dict], None]):
        self.message_handlers[msg_type] = handler
    
    def _call_handler(self, msg_type: str, message: dict):
        if msg_type in self.message_handlers:
            try:
                self.message_handlers[msg_type](message)
            except Exception as e:
                self.logger.error(f"Handler error for {msg_type}: {e}")

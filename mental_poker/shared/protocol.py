"""
Protocol definitions for Mental Poker communication
"""

from enum import Enum
import json

class MessageType(Enum):
    # Connection messages
    REGISTER = "register"
    REGISTER_RESPONSE = "register_response"
    
    # Session management
    CREATE_SESSION = "create_session"
    SESSION_CREATED = "session_created"
    JOIN_SESSION = "join_session"
    SESSION_JOINED = "session_joined"
    PLAYER_JOINED = "player_joined"
    PLAYER_LEFT = "player_left"
    
    # Game flow
    START_GAME = "start_game"
    GAME_STARTED = "game_started"
    ENCRYPTION_DONE = "encryption_done"
    START_CARD_DEALING = "start_card_dealing"
    
    # Card operations
    CARD_SELECTION = "card_selection"
    PARTIAL_DECRYPT = "partial_decrypt"
    
    # Betting
    BET_ACTION = "bet_action"
    
    # Chat
    CHAT_MESSAGE = "chat_message"
    
    # System
    ERROR = "error"
    DISCONNECTED = "disconnected"

class PokerProtocol:
    @staticmethod
    def create_message(msg_type: MessageType, **kwargs):
        message = {'type': msg_type.value}
        message.update(kwargs)
        return json.dumps(message)
    
    @staticmethod
    def parse_message(data):
        return json.loads(data)

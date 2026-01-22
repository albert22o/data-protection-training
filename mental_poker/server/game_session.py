import logging
from typing import List, Dict, Any

class GameSession:
    def __init__(self, session_id: str, name: str, max_players: int = 6):
        self.session_id = session_id
        self.name = name
        self.max_players = max_players
        self.players: Dict[str, Dict] = {}  # player_id -> {name, ready, encrypted_deck}
        self.game_state = {
            'phase': 'waiting',  # waiting, dealing, preflop, flop, turn, river, finished
            'community_cards': [],
            'pot': 0,
            'current_player': None,
            'dealer_position': 0
        }
        self.logger = logging.getLogger(f'Session-{session_id}')
    
    def add_player(self, player_id: str, player_name: str) -> bool:
        if len(self.players) >= self.max_players:
            return False
        
        self.players[player_id] = {
            'name': player_name,
            'ready': False,
            'encrypted_deck': None,
            'chips': 1000,
            'cards': [],
            'folded': False
        }
        self.logger.info(f"Player added: {player_name} ({player_id})")
        return True
    
    def remove_player(self, player_id: str):
        if player_id in self.players:
            player_name = self.players[player_id]['name']
            del self.players[player_id]
            self.logger.info(f"Player removed: {player_name} ({player_id})")
    
    def get_players_info(self) -> List[Dict]:
        return [{
            'id': pid,
            'name': info['name'],
            'ready': info['ready'],
            'chips': info['chips']
        } for pid, info in self.players.items()]
    
    def set_player_ready(self, player_id: str, encrypted_deck: List) -> bool:
        if player_id in self.players:
            self.players[player_id]['ready'] = True
            self.players[player_id]['encrypted_deck'] = encrypted_deck
            
            # Проверяем, все ли игроки готовы
            all_ready = all(player['ready'] for player in self.players.values())
            if all_ready and len(self.players) >= 2:
                return True
        
        return False
    
    def start_game(self) -> bool:
        if len(self.players) < 2:
            return False
        
        self.game_state['phase'] = 'dealing'
        self.logger.info("Game started")
        return True

    def get_game_state(self) -> Dict:
        players_info = []
        for pid, info in self.players.items():
            players_info.append({
                'id': pid,
                'name': info['name'],
                'chips': info['chips'],
                'cards': info.get('cards', []),
                'folded': info.get('folded', False)
            })

        return {
            **self.game_state,
            'players': players_info
        }

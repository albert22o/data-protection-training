import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import time
from client.poker_client import PokerClient
from client.mental_poker import MentalPoker

class PokerGUI:
    def __init__(self, root):
        self.root = root
        self.client = PokerClient()
        self.poker_game = None
        self.player_cards = []
        self.community_cards = []
        
        self.setup_gui()
        self.setup_client_handlers()
    
    def setup_gui(self):
        self.root.title("Mental Poker - Texas Hold'em")
        self.root.geometry("1200x800")
        
        # Создаем вкладки
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill='both', expand=True, padx=10, pady=10)
        
        # Вкладка подключения
        self.setup_connection_tab()
        
        # Вкладка лобби
        self.setup_lobby_tab()
        
        # Вкладка игры
        self.setup_game_tab()
        
        # Вкладка чата
        self.setup_chat_tab()
    
    def setup_connection_tab(self):
        self.connection_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.connection_frame, text="Connection")
        
        ttk.Label(self.connection_frame, text="Mental Poker Client", 
                 font=('Arial', 16)).pack(pady=20)
        
        # Параметры подключения
        conn_params = ttk.Frame(self.connection_frame)
        conn_params.pack(pady=20)
        
        ttk.Label(conn_params, text="Server:").grid(row=0, column=0, sticky='w', padx=5, pady=5)
        self.server_entry = ttk.Entry(conn_params, width=20)
        self.server_entry.insert(0, "localhost")
        self.server_entry.grid(row=0, column=1, padx=5, pady=5)
        
        ttk.Label(conn_params, text="Port:").grid(row=1, column=0, sticky='w', padx=5, pady=5)
        self.port_entry = ttk.Entry(conn_params, width=10)
        self.port_entry.insert(0, "8888")
        self.port_entry.grid(row=1, column=1, padx=5, pady=5)
        
        ttk.Label(conn_params, text="Player Name:").grid(row=2, column=0, sticky='w', padx=5, pady=5)
        self.name_entry = ttk.Entry(conn_params, width=20)
        self.name_entry.insert(0, f"Player_{int(time.time()) % 1000}")
        self.name_entry.grid(row=2, column=1, padx=5, pady=5)
        
        self.connect_btn = ttk.Button(conn_params, text="Connect", 
                                    command=self.connect_to_server)
        self.connect_btn.grid(row=3, column=0, columnspan=2, pady=10)
        
        # Статус подключения
        self.status_label = ttk.Label(self.connection_frame, text="Disconnected", 
                                     foreground="red")
        self.status_label.pack(pady=10)
    
    def setup_lobby_tab(self):
        self.lobby_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.lobby_frame, text="Lobby")
        
        # Создание сессии
        session_frame = ttk.LabelFrame(self.lobby_frame, text="Game Session", padding=10)
        session_frame.pack(fill='x', padx=10, pady=10)
        
        ttk.Label(session_frame, text="Session Name:").grid(row=0, column=0, sticky='w')
        self.session_name_entry = ttk.Entry(session_frame, width=20)
        self.session_name_entry.insert(0, "Texas Hold'em Game")
        self.session_name_entry.grid(row=0, column=1, padx=5)
        
        self.create_session_btn = ttk.Button(session_frame, text="Create Session", 
                                           command=self.create_session, state="disabled")
        self.create_session_btn.grid(row=0, column=2, padx=5)
        
        # Присоединение к сессии
        join_frame = ttk.LabelFrame(self.lobby_frame, text="Join Session", padding=10)
        join_frame.pack(fill='x', padx=10, pady=10)
        
        ttk.Label(join_frame, text="Session ID:").grid(row=0, column=0, sticky='w')
        self.session_id_entry = ttk.Entry(join_frame, width=15)
        self.session_id_entry.grid(row=0, column=1, padx=5)
        
        self.join_session_btn = ttk.Button(join_frame, text="Join Session", 
                                         command=self.join_session, state="disabled")
        self.join_session_btn.grid(row=0, column=2, padx=5)
        
        # Список игроков
        players_frame = ttk.LabelFrame(self.lobby_frame, text="Players", padding=10)
        players_frame.pack(fill='both', expand=True, padx=10, pady=10)
        
        self.players_tree = ttk.Treeview(players_frame, columns=('name', 'status'), show='headings')
        self.players_tree.heading('name', text='Player Name')
        self.players_tree.heading('status', text='Status')
        self.players_tree.pack(fill='both', expand=True)
        
        # Управление игрой
        game_control_frame = ttk.Frame(self.lobby_frame)
        game_control_frame.pack(fill='x', padx=10, pady=10)
        
        self.start_game_btn = ttk.Button(game_control_frame, text="Start Game", 
                                       command=self.start_game, state="disabled")
        self.start_game_btn.pack(side='right')
    
    def setup_game_tab(self):
        self.game_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.game_frame, text="Game")
        
        # Игровой стол
        table_frame = ttk.LabelFrame(self.game_frame, text="Poker Table", padding=20)
        table_frame.pack(fill='both', expand=True, padx=10, pady=10)
        
        # Общие карты
        self.community_cards_frame = ttk.Frame(table_frame)
        self.community_cards_frame.pack(pady=20)
        ttk.Label(self.community_cards_frame, text="Community Cards:", 
                 font=('Arial', 12)).pack()
        
        self.community_cards_display = ttk.Frame(self.community_cards_frame)
        self.community_cards_display.pack(pady=10)
        
        # Карты игрока
        self.player_cards_frame = ttk.Frame(table_frame)
        self.player_cards_frame.pack(pady=20)
        ttk.Label(self.player_cards_frame, text="Your Cards:", 
                 font=('Arial', 12)).pack()
        
        self.player_cards_display = ttk.Frame(self.player_cards_frame)
        self.player_cards_display.pack(pady=10)
        
        # Кнопки для игры
        self.actions_frame = ttk.Frame(table_frame)
        self.actions_frame.pack(pady=20)
        
        ttk.Button(self.actions_frame, text="Fold", 
                  command=lambda: self.send_bet_action('fold')).grid(row=0, column=0, padx=5)
        ttk.Button(self.actions_frame, text="Check", 
                  command=lambda: self.send_bet_action('check')).grid(row=0, column=1, padx=5)
        ttk.Button(self.actions_frame, text="Call", 
                  command=lambda: self.send_bet_action('call')).grid(row=0, column=2, padx=5)
        ttk.Button(self.actions_frame, text="Raise", 
                  command=lambda: self.send_bet_action('raise')).grid(row=0, column=3, padx=5)
        
        self.bet_amount_entry = ttk.Entry(self.actions_frame, width=8)
        self.bet_amount_entry.insert(0, "50")
        self.bet_amount_entry.grid(row=0, column=4, padx=5)
        
        # Статус игры
        self.game_status_label = ttk.Label(table_frame, text="Game not started", 
                                          font=('Arial', 10))
        self.game_status_label.pack(pady=10)
    
    def setup_chat_tab(self):
        self.chat_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.chat_frame, text="Chat")
        
        # История чата
        self.chat_history = scrolledtext.ScrolledText(self.chat_frame, height=20, state='disabled')
        self.chat_history.pack(fill='both', expand=True, padx=10, pady=10)
        
        # Ввод сообщения
        chat_input_frame = ttk.Frame(self.chat_frame)
        chat_input_frame.pack(fill='x', padx=10, pady=10)
        
        self.chat_entry = ttk.Entry(chat_input_frame)
        self.chat_entry.pack(side='left', fill='x', expand=True, padx=(0, 5))
        self.chat_entry.bind('<Return>', self.send_chat_message)
        
        ttk.Button(chat_input_frame, text="Send", 
                  command=self.send_chat_message).pack(side='right')

    def setup_client_handlers(self):
        # Регистрируем обработчики сообщений от сервера
        self.client.register_handler('register_response', self.handle_register_response)
        self.client.register_handler('session_created', self.handle_session_created)
        self.client.register_handler('session_joined', self.handle_session_joined)
        self.client.register_handler('player_joined', self.handle_player_joined)
        self.client.register_handler('player_left', self.handle_player_left)
        self.client.register_handler('game_started', self.handle_game_started)
        self.client.register_handler('encryption_acknowledged', self.handle_encryption_acknowledged)
        self.client.register_handler('start_card_dealing', self.handle_start_card_dealing)
        self.client.register_handler('card_dealing_complete', self.handle_card_dealing_complete)
        self.client.register_handler('game_state_update', self.handle_game_state_update)
        self.client.register_handler('session_list', self.handle_session_list)
        self.client.register_handler('session_list_update', self.handle_session_list)
        self.client.register_handler('chat_message', self.handle_chat_message)
        self.client.register_handler('disconnected', self.handle_disconnected)

    def handle_encryption_acknowledged(self, message):
        """Обработчик подтверждения шифрования от сервера"""
        self.game_status_label.config(text="Encryption acknowledged by server - waiting for other players...")

    def handle_card_dealing_complete(self, message):
        """Обработчик завершения раздачи карт"""
        player_cards = message.get('player_cards', {})
        community_cards = message.get('community_cards', [])

        # Получаем карты текущего игрока
        if self.client.player_id in player_cards:
            self.player_cards = player_cards[self.client.player_id]

        self.community_cards = community_cards
        self.update_cards_display()

        self.game_status_label.config(text="Cards dealt! Your turn to bet.")
        self.add_chat_message("System", "Cards have been dealt!")

    def handle_game_state_update(self, message):
        """Обработчик обновления состояния игры"""
        game_state = message.get('game_state', {})
        self.update_game_state(game_state)

    def handle_session_list(self, message):
        """Обработчик получения списка сессий"""
        sessions = message.get('sessions', [])
        self.update_sessions_list(sessions)

    def update_sessions_list(self, sessions):
        """Обновляет список доступных сессий"""
        # Эта функция будет обновлять UI со списком сессий
        print(f"Available sessions: {len(sessions)}")
        for session in sessions:
            print(f"  - {session['name']} ({session['players_count']}/{session['max_players']})")

    def connect_to_server(self):
        server = self.server_entry.get()
        port = self.port_entry.get()
        name = self.name_entry.get()

        if not name:
            messagebox.showerror("Error", "Please enter your name")
            return

        if not port:
            port = "8888"

        try:
            port = int(port)
        except ValueError:
            messagebox.showerror("Error", "Invalid port number")
            return

        if self.client.connect(server, port, name):
            self.connect_btn.config(state="disabled")
            self.status_label.config(text="Connecting...", foreground="orange")

            # Запрашиваем список сессий после подключения
            threading.Timer(1.0, self.request_sessions_list).start()
        else:
            messagebox.showerror("Connection Error", "Failed to connect to server")

    def request_sessions_list(self):
        """Запрашивает список сессий у сервера"""
        if self.client.connected:
            self.client.send_message('get_sessions')

    def handle_register_response(self, message):
        if message.get('status') == 'success':
            self.status_label.config(text="Connected", foreground="green")
            self.create_session_btn.config(state="normal")
            self.join_session_btn.config(state="normal")
            self.notebook.select(1)  # Переключаем на вкладку лобби

            # Запрашиваем актуальный список сессий
            self.request_sessions_list()
        else:
            messagebox.showerror("Registration Error", "Failed to register with server")

    def create_session(self):
        session_name = self.session_name_entry.get()
        if session_name:
            self.client.send_message('create_session', session_name=session_name)
    
    def handle_session_created(self, message):
        session_id = message.get('session_id')
        self.session_id_entry.delete(0, tk.END)
        self.session_id_entry.insert(0, session_id)
        messagebox.showinfo("Session Created", f"Session created with ID: {session_id}")
    
    def join_session(self):
        session_id = self.session_id_entry.get()
        if session_id:
            self.client.send_message('join_session', session_id=session_id)
    
    def handle_session_joined(self, message):
        players = message.get('players', [])
        self.update_players_list(players)
        self.start_game_btn.config(state="normal")
        self.add_chat_message("System", "Joined game session")
    
    def handle_player_joined(self, message):
        player_name = message.get('player_name')
        players = message.get('players', [])
        self.update_players_list(players)
        self.add_chat_message("System", f"{player_name} joined the game")
    
    def handle_player_left(self, message):
        player_name = message.get('player_name')
        players = message.get('players', [])
        self.update_players_list(players)
        self.add_chat_message("System", f"{player_name} left the game")
    
    def start_game(self):
        self.client.send_message('start_game')
        self.notebook.select(2)  # Переключаем на вкладку игры
    
    def handle_game_started(self, message):
        self.game_status_label.config(text="Game started - encrypting deck...")
        self.add_chat_message("System", "Game started! Encrypting deck...")
        
        # Инициализируем ментальный покер
        # В реальной реализации количество игроков должно быть известно
        self.poker_game = MentalPoker(num_players=2)  # Временно 2 игрока
        
        # Шифруем колоду и отправляем на сервер
        encrypted_deck = self.poker_game.encrypt_deck()
        self.client.send_message('encryption_done', encrypted_deck=encrypted_deck)
    
    def handle_start_card_dealing(self, message):
        self.game_status_label.config(text="Dealing cards...")
        self.add_chat_message("System", "Dealing cards using mental poker protocol")
        
        # В реальной реализации здесь должна быть сложная логика распределения карт
        # Для демонстрации просто покажем примерные карты
        if self.poker_game:
            # Симуляция раздачи карт
            self.player_cards = ['A♠', 'K♥']  # Пример карт
            self.community_cards = ['2♠', '7♥', 'Q♦', '10♣', '3♠']  # Пример общих карт
            
            self.update_cards_display()
    
    def send_bet_action(self, action):
        amount = 0
        if action == 'raise':
            try:
                amount = int(self.bet_amount_entry.get())
            except ValueError:
                messagebox.showerror("Error", "Invalid bet amount")
                return
        
        self.client.send_message('bet_action', action=action, amount=amount)
        self.add_chat_message("You", f"{action.capitalize()} {amount if amount else ''}")

    def send_chat_message(self, event=None):
        message = self.chat_entry.get().strip()
        if message:
            # Сразу добавляем свое сообщение в чат
            self.add_chat_message("You", message)
            self.client.send_message('chat_message', message=message)
            self.chat_entry.delete(0, tk.END)

    def handle_chat_message(self, message):
        player_name = message.get('player_name', 'Unknown')
        chat_message = message.get('message', '')
        # Не добавляем сообщение если это наше собственное (мы уже добавили его)
        if player_name != "You":
            self.add_chat_message(player_name, chat_message)
    
    def handle_disconnected(self, message):
        self.status_label.config(text="Disconnected", foreground="red")
        self.connect_btn.config(state="normal")
        messagebox.showerror("Disconnected", "Lost connection to server")
    
    def update_players_list(self, players):
        self.players_tree.delete(*self.players_tree.get_children())
        for player in players:
            status = "Ready" if player.get('ready', False) else "Waiting"
            self.players_tree.insert('', 'end', values=(player['name'], status))
    
    def update_cards_display(self):
        # Очищаем предыдущие карты
        for widget in self.player_cards_display.winfo_children():
            widget.destroy()
        for widget in self.community_cards_display.winfo_children():
            widget.destroy()
        
        # Отображаем карты игрока
        for card in self.player_cards:
            card_label = ttk.Label(self.player_cards_display, text=card, 
                                 font=('Arial', 16), background='white', 
                                 relief='solid', padding=10)
            card_label.pack(side='left', padx=5)
        
        # Отображаем общие карты
        for card in self.community_cards:
            card_label = ttk.Label(self.community_cards_display, text=card, 
                                 font=('Arial', 16), background='white', 
                                 relief='solid', padding=10)
            card_label.pack(side='left', padx=5)
    
    def add_chat_message(self, sender, message):
        self.chat_history.config(state='normal')
        self.chat_history.insert('end', f"{sender}: {message}\n")
        self.chat_history.config(state='disabled')
        self.chat_history.see('end')

#!/usr/bin/env python3
import sys
import os
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from server.poker_server import PokerServer
import logging

def main():
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )
    
    server = PokerServer(host='localhost', port=8888)
    print("Mental Poker Server starting...")
    print("Host: localhost:8888")
    print("Press Ctrl+C to stop the server")
    
    try:
        server.start()
    except KeyboardInterrupt:
        print("\nServer stopped by user")
    except Exception as e:
        print(f"Server error: {e}")

if __name__ == "__main__":
    main()

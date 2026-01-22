#!/usr/bin/env python3
import sys
import os
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from client.poker_gui import PokerGUI
import tkinter as tk

def main():
    root = tk.Tk()
    app = PokerGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main()

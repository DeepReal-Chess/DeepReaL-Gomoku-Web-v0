// ============================================================================
// DeepReaL Gomoku Engine v0 - UCI Interface
// ============================================================================

#include "uci.h"
#include "board.h"
#include "search.h"
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

// Global board state
static Board g_board;

// Helper: Display board
static void display_board() {
    cout << "   ";
    for (int c = 0; c < 15; c++) cout << (c < 10 ? " " : "") << c;
    cout << endl;
    for (int r = 0; r < 15; r++) {
        cout << (r < 10 ? " " : "") << r << " ";
        for (int c = 0; c < 15; c++) {
            int idx = r * 15 + c;
            if (g_board.b[0].get(idx)) cout << " X";
            else if (g_board.b[1].get(idx)) cout << " O";
            else cout << " .";
        }
        cout << endl;
    }
    cout << "Move count: " << g_board.cnt << ", ";
    cout << (g_board.side() == 0 ? "Black" : "White") << " to move" << endl;
    if (g_board.res == 1) cout << "Result: Black wins" << endl;
    else if (g_board.res == 0) cout << "Result: White wins" << endl;
}

void uci_loop() {
    string line;
    
    while (getline(cin, line)) {
        istringstream iss(line);
        string cmd;
        iss >> cmd;
        
        if (cmd == "uci") {
            cout << "id name DeepReaL Gomoku v0" << endl;
            cout << "id author DeepReaL" << endl;
            cout << "uciok" << endl;
        }
        else if (cmd == "init") {
            init_precompute();
            init_threat_tables();
            g_board.init();
            cout << "ready" << endl;
        }
        else if (cmd == "update") {
            int index;
            if (iss >> index) {
                if (index >= 0 && index < 225 && g_board.res == -1) {
                    g_board.set(index);
                    cout << "ok" << endl;
                } else {
                    cout << "error: invalid move" << endl;
                }
            } else {
                cout << "error: missing index" << endl;
            }
        }
        else if (cmd == "go") {
            int iters = 10000;  // default
            iss >> iters;
            
            if (g_board.res != -1) {
                cout << "error: game already ended" << endl;
            } else {
                int best = getBest(g_board, iters);
                cout << "bestmove " << best << endl;
            }
        }
        else if (cmd == "d") {
            display_board();
        }
        else if (cmd == "quit") {
            break;
        }
        else if (!cmd.empty()) {
            cout << "unknown command: " << cmd << endl;
        }
    }
}

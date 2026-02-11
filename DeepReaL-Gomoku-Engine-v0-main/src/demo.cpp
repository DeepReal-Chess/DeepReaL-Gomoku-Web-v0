// ============================================================================
// DeepReaL Gomoku Engine v0 - Demo
// Supports: Bot vs Bot, Human vs Bot
// ============================================================================

#include "board.h"
#include "search.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

// Helper: Display board
void display_board(const Board& board) {
    cout << "  ";
    for (int c = 0; c < 15; c++) cout << " " << (c % 10);
    cout << endl;
    for (int r = 0; r < 15; r++) {
        cout << (r % 10) << " ";
        for (int c = 0; c < 15; c++) {
            int idx = r * 15 + c;
            if (board.b[0].get(idx)) cout << " X";
            else if (board.b[1].get(idx)) cout << " O";
            else cout << " .";
        }
        cout << endl;
    }
}

// Helper: Convert index to (row, col) string
string idx_to_coord(int idx) {
    return "(" + to_string(idx / 15) + "," + to_string(idx % 15) + ")";
}

// Helper: Parse user input (row,col) or (row col) or row col
int parse_move(const string& input) {
    int row = -1, col = -1;
    
    // Try parsing "(row,col)" format
    if (sscanf(input.c_str(), "(%d,%d)", &row, &col) == 2) {
        // parsed
    }
    // Try parsing "row,col" format
    else if (sscanf(input.c_str(), "%d,%d", &row, &col) == 2) {
        // parsed
    }
    // Try parsing "row col" format
    else if (sscanf(input.c_str(), "%d %d", &row, &col) == 2) {
        // parsed
    }
    // Try parsing single number as index
    else {
        int idx;
        if (sscanf(input.c_str(), "%d", &idx) == 1 && idx >= 0 && idx < 225) {
            return idx;
        }
    }
    
    if (row >= 0 && row < 15 && col >= 0 && col < 15) {
        return row * 15 + col;
    }
    return -1;
}

// Generate and print PGN
void print_pgn(const vector<int>& moves, int result, bool human_played_black) {
    cout << endl << "=== PGN ===" << endl;
    cout << "[black] " << (human_played_black ? "Human" : "DeepReaL v0") << endl;
    cout << "[white] " << (human_played_black ? "DeepReaL v0" : "Human") << endl;
    
    if (result == 1) {
        cout << "[result] 1-0" << endl;
    } else if (result == 0) {
        cout << "[result] 0-1" << endl;
    } else {
        cout << "[result] -" << endl;
    }
    
    for (size_t i = 0; i < moves.size(); i += 2) {
        cout << (i/2 + 1) << ". " << idx_to_coord(moves[i]);
        if (i + 1 < moves.size()) {
            cout << " " << idx_to_coord(moves[i + 1]);
        }
        if ((i/2 + 1) % 8 == 0) cout << endl;
        else cout << " ";
    }
    cout << endl;
}

// Bot vs Bot game
void bot_vs_bot() {
    cout << "=== Bot vs Bot ===" << endl;
    cout << endl;
    
    Board board;
    board.init();
    
    vector<int> moves;
    int iters = 270000;  // ~3 sec per move at ~90K iter/sec
    
    cout << "Starting self-play game with " << iters << " iterations per move (~3 sec)..." << endl;
    cout << endl;
    
    while (!board.isTerminal() && board.cnt < 225) {
        int move = getBest(board, iters);
        
        if (move == -1) {
            cout << "No legal moves available!" << endl;
            break;
        }
        
        moves.push_back(move);
        board.set(move);
        
        cout << "Move " << board.cnt << ": " 
             << (board.cnt % 2 == 1 ? "Black" : "White") << " plays "
             << idx_to_coord(move) << endl;
        display_board(board);
        cout << endl;
    }
    
    cout << "=== Game Over ===" << endl;
    if (board.res == 1) {
        cout << "Result: Black wins!" << endl;
    } else if (board.res == 0) {
        cout << "Result: White wins!" << endl;
    } else {
        cout << "Result: Draw" << endl;
    }
    
    print_pgn(moves, board.res, false);
}

// Human vs Bot game
void human_vs_bot(bool human_is_black) {
    cout << "=== Human vs Bot ===" << endl;
    cout << "You are playing as " << (human_is_black ? "Black (X)" : "White (O)") << endl;
    cout << "Enter moves as: row,col  or  (row,col)  or  row col" << endl;
    cout << "Coordinates are 0-indexed (0-14)" << endl;
    cout << "Type 'quit' to exit" << endl;
    cout << endl;
    
    Board board;
    board.init();
    
    vector<int> moves;
    int iters = 270000;  // ~3 sec per move
    
    display_board(board);
    cout << endl;
    
    while (!board.isTerminal() && board.cnt < 225) {
        bool is_black_turn = (board.cnt % 2 == 0);
        bool human_turn = (is_black_turn == human_is_black);
        
        int move = -1;
        
        if (human_turn) {
            // Human's turn
            cout << "Your move " << (is_black_turn ? "(Black/X)" : "(White/O)") << ": ";
            string input;
            getline(cin, input);
            
            if (input == "quit" || input == "exit" || input == "q") {
                cout << "Game aborted." << endl;
                return;
            }
            
            move = parse_move(input);
            
            // Validate move
            if (move < 0 || move >= 225) {
                cout << "Invalid input. Use format: row,col (e.g., 7,7)" << endl;
                continue;
            }
            
            if ((board.b[0] | board.b[1]).get(move)) {
                cout << "Position " << idx_to_coord(move) << " is already occupied!" << endl;
                continue;
            }
        } else {
            // Bot's turn
            cout << "Bot is thinking..." << endl;
            move = getBest(board, iters);
            
            if (move == -1) {
                cout << "Bot has no legal moves!" << endl;
                break;
            }
            
            cout << "Bot plays: " << idx_to_coord(move) << endl;
        }
        
        moves.push_back(move);
        board.set(move);
        
        cout << endl;
        cout << "Move " << board.cnt << ": " 
             << (board.cnt % 2 == 1 ? "Black" : "White") << " at "
             << idx_to_coord(move) << endl;
        display_board(board);
        cout << endl;
    }
    
    cout << "=== Game Over ===" << endl;
    if (board.res == 1) {
        cout << (human_is_black ? "You win!" : "Bot wins!") << endl;
    } else if (board.res == 0) {
        cout << (human_is_black ? "Bot wins!" : "You win!") << endl;
    } else {
        cout << "Draw!" << endl;
    }
    
    print_pgn(moves, board.res, human_is_black);
}

int main() {
    cout << "========================================" << endl;
    cout << "  DeepReaL Gomoku Engine v0 - Demo" << endl;
    cout << "========================================" << endl;
    cout << endl;
    
    // Initialize
    init_precompute();
    init_threat_tables();
    
    cout << "Select game mode:" << endl;
    cout << "  1. Bot vs Bot (watch self-play)" << endl;
    cout << "  2. Human vs Bot (you play Black/X)" << endl;
    cout << "  3. Human vs Bot (you play White/O)" << endl;
    cout << endl;
    cout << "Enter choice (1-3): ";
    
    string choice;
    getline(cin, choice);
    
    cout << endl;
    
    if (choice == "1") {
        bot_vs_bot();
    } else if (choice == "2") {
        human_vs_bot(true);  // Human is black
    } else if (choice == "3") {
        human_vs_bot(false); // Human is white
    } else {
        cout << "Invalid choice. Running Bot vs Bot by default." << endl;
        cout << endl;
        bot_vs_bot();
    }
    
    return 0;
}

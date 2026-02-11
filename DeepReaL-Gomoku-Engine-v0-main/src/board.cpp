#include "board.h"

// ============================================================================
// Board::init - Clear the board to initial state
// ============================================================================
void Board::init() {
    b[0] = lbit256();
    b[1] = lbit256();
    nxt = lbit256();
    
    for (int i = 0; i < 15; i++) {
        row[0][i] = row[1][i] = 0;
        col[0][i] = col[1][i] = 0;
    }
    for (int i = 0; i < 29; i++) {
        diag1[0][i] = diag1[1][i] = 0;
        diag2[0][i] = diag2[1][i] = 0;
    }
    
    cnt = 0;
    res = -1;  // Game ongoing
    last_move = -1;  // No moves yet
}

// ============================================================================
// Board::set - Make a move at the given index
// ============================================================================
void Board::set(int index) {
    int side = cnt & 1;  // 0 = black, 1 = white
    int r = row_id[index];
    int c = col_id[index];

    // Update bitboard
    b[side] |= f[index];

    // Update line masks for this player
    row[side][r] |= (1 << c);
    col[side][c] |= (1 << r);
    diag1[side][diag1_idx[index]] |= (1 << diag1_idy[index]);
    diag2[side][diag2_idx[index]] |= (1 << diag2_idy[index]);

    // Update next legal moves
    nxt |= cheb2[index];
    nxt &= ~b[0];
    nxt &= ~b[1];
    // Also mask to valid board positions (bits 0-224 only)
    nxt &= board_mask;

    // Check for win using current player's line masks
    if (win_table[row[side][r]] || 
        win_table[col[side][c]] || 
        win_table[diag1[side][diag1_idx[index]]] || 
        win_table[diag2[side][diag2_idx[index]]]) {
        // Current player (side) wins
        // res = 1 means black wins, res = 0 means white wins
        res = (side == 0) ? 1 : 0;
    }

    last_move = index;  // Track the last move
    cnt++;
}

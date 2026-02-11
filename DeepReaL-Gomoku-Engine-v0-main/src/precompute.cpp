#include "board.h"
#include <algorithm>

// ============================================================================
// Global precomputed tables
// ============================================================================
int row_id[225];
int col_id[225];
int diag1_idx[225];
int diag1_idy[225];
int diag2_idx[225];
int diag2_idy[225];
lbit256 f[225];
lbit256 cheb2[225];
bool win_table[1 << 15];
lbit256 board_mask;

// ============================================================================
// Helper: Check if (row, col) is within board bounds
// ============================================================================
static inline bool in_bounds(int r, int c) {
    return r >= 0 && r < 15 && c >= 0 && c < 15;
}

// ============================================================================
// Initialize all precomputed tables
// ============================================================================
void init_precompute() {
    // Initialize index tables
    for (int i = 0; i < 225; i++) {
        int r = i / 15;
        int c = i % 15;
        row_id[i] = r;
        col_id[i] = c;
        diag1_idx[i] = r - c + 14;  // Range: 0-28
        diag1_idy[i] = std::min(r, c);
        diag2_idx[i] = r + c;       // Range: 0-28
        diag2_idy[i] = std::min(r, 14 - c);
    }

    // Initialize f[i] - single bit at position i
    for (int i = 0; i < 225; i++) {
        f[i] = lbit256();
        f[i].set(i);
    }

    // Initialize board_mask - all valid positions (0-224)
    board_mask = lbit256();
    for (int i = 0; i < 225; i++) {
        board_mask.set(i);
    }

    // Initialize cheb2[i] - Chebyshev distance 2 neighborhood
    // This is a 5x5 box centered at i, excluding i itself
    for (int i = 0; i < 225; i++) {
        cheb2[i] = lbit256();
        int r = row_id[i];
        int c = col_id[i];

        for (int dr = -2; dr <= 2; dr++) {
            for (int dc = -2; dc <= 2; dc++) {
                if (dr == 0 && dc == 0) continue;  // Exclude center
                int nr = r + dr;
                int nc = c + dc;
                if (in_bounds(nr, nc)) {
                    int nidx = nr * 15 + nc;
                    cheb2[i].set(nidx);
                }
            }
        }
    }

    // Initialize win_table - true if pattern has 5+ consecutive bits
    for (int mask = 0; mask < (1 << 15); mask++) {
        win_table[mask] = false;
        int consecutive = 0;
        for (int bit = 0; bit < 15; bit++) {
            if ((mask >> bit) & 1) {
                consecutive++;
                if (consecutive >= 5) {
                    win_table[mask] = true;
                    break;
                }
            } else {
                consecutive = 0;
            }
        }
    }
}

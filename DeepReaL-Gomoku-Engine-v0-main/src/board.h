#ifndef BOARD_H
#define BOARD_H

#include <cstdint>
#include <cstring>

// ============================================================================
// lbit256: 256-bit integer using 4 unsigned long long
// ============================================================================
struct lbit256 {
    unsigned long long d[4];  // d[0]: bits 0-63, d[1]: 64-127, d[2]: 128-191, d[3]: 192-255

    inline lbit256() : d{0, 0, 0, 0} {}
    
    inline lbit256(unsigned long long d0, unsigned long long d1, 
                   unsigned long long d2, unsigned long long d3) 
        : d{d0, d1, d2, d3} {}

    // Bitwise OR
    inline lbit256 operator|(const lbit256& o) const {
        return lbit256(d[0] | o.d[0], d[1] | o.d[1], d[2] | o.d[2], d[3] | o.d[3]);
    }

    // Bitwise AND
    inline lbit256 operator&(const lbit256& o) const {
        return lbit256(d[0] & o.d[0], d[1] & o.d[1], d[2] & o.d[2], d[3] & o.d[3]);
    }

    // Bitwise NOT
    inline lbit256 operator~() const {
        return lbit256(~d[0], ~d[1], ~d[2], ~d[3]);
    }

    // Bitwise OR assignment
    inline lbit256& operator|=(const lbit256& o) {
        d[0] |= o.d[0]; d[1] |= o.d[1]; d[2] |= o.d[2]; d[3] |= o.d[3];
        return *this;
    }

    // Bitwise AND assignment
    inline lbit256& operator&=(const lbit256& o) {
        d[0] &= o.d[0]; d[1] &= o.d[1]; d[2] &= o.d[2]; d[3] &= o.d[3];
        return *this;
    }

    // Check if all bits are zero
    inline bool empty() const {
        return (d[0] | d[1] | d[2] | d[3]) == 0;
    }

    // Count set bits
    inline int popcount() const {
        return __builtin_popcountll(d[0]) + __builtin_popcountll(d[1]) +
               __builtin_popcountll(d[2]) + __builtin_popcountll(d[3]);
    }

    // Set bit at index
    inline void set(int idx) {
        d[idx >> 6] |= 1ULL << (idx & 63);
    }

    // Clear bit at index
    inline void clear(int idx) {
        d[idx >> 6] &= ~(1ULL << (idx & 63));
    }

    // Get bit at index
    inline bool get(int idx) const {
        return (d[idx >> 6] >> (idx & 63)) & 1;
    }

    // Pop the lowest set bit and return its index
    // Returns -1 if empty
    inline int popbit() {
        if (d[0]) {
            int idx = __builtin_ctzll(d[0]);
            d[0] &= d[0] - 1;  // Clear lowest bit
            return idx;
        }
        if (d[1]) {
            int idx = __builtin_ctzll(d[1]);
            d[1] &= d[1] - 1;
            return 64 + idx;
        }
        if (d[2]) {
            int idx = __builtin_ctzll(d[2]);
            d[2] &= d[2] - 1;
            return 128 + idx;
        }
        if (d[3]) {
            int idx = __builtin_ctzll(d[3]);
            d[3] &= d[3] - 1;
            return 192 + idx;
        }
        return -1;
    }
};

// ============================================================================
// Board: Gomoku board representation
// ============================================================================
struct Board {
    lbit256 b[2];       // b[0] = black, b[1] = white
    lbit256 nxt;        // Next legal moves (Chebyshev-2 neighborhood of all pieces)
    int row[2][15];     // Row line masks per player (15 bits each)
    int col[2][15];     // Column line masks per player
    int diag1[2][29];   // Main diagonal masks per player (row - col + 14)
    int diag2[2][29];   // Anti-diagonal masks per player (row + col)
    int cnt;            // Move count (cnt & 1 == 0 means black to move)
    int res;            // Result: -1 = ongoing, 0 = white win, 1 = black win
    int last_move;      // Last move played (-1 if none)

    void init();
    void set(int index);
    
    inline bool isTerminal() const { return res != -1; }
    inline int side() const { return cnt & 1; }  // 0 = black, 1 = white
};

// ============================================================================
// Precomputed tables (declared here, defined in precompute.cpp)
// ============================================================================
extern int row_id[225];
extern int col_id[225];
extern int diag1_idx[225];
extern int diag1_idy[225];
extern int diag2_idx[225];
extern int diag2_idy[225];
extern lbit256 f[225];
extern lbit256 cheb2[225];
extern bool win_table[1 << 15];

// Mask for valid board positions (bits 0-224)
extern lbit256 board_mask;

// Initialize all precomputed tables
void init_precompute();

#endif // BOARD_H

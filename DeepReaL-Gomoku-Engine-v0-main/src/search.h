#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include <cmath>

// ============================================================================
// Threat levels
// ============================================================================
constexpr int THREAT_NONE = 0;
constexpr int THREAT_LIVE3_THREAT = 1;  // Opponent's live3, should block
constexpr int THREAT_LIVE3_WIN = 2;     // Our live3, forcing move
constexpr int THREAT_OPEN4_THREAT = 3;  // Opponent's open4, must block
constexpr int THREAT_OPEN4_WIN = 4;     // Our open4, immediate win

// ============================================================================
// Threat tables (precomputed)
// threat_table[mask][pos] - threat level if we play at pos in line with mask
// winning_table[mask][pos] - winning move level (for live3, middle gap is winning)
// ============================================================================
extern int8_t threat_table[1 << 15][15];
extern int8_t winning_table[1 << 15][15];

// Initialize threat tables
void init_threat_tables();

// ============================================================================
// Node structure for MCTS tree
// ============================================================================
constexpr int MAX_NODES = 10000000;  // 10M nodes

struct Node {
    int fa;           // Parent index (-1 for root)
    int fst_child;    // First child index (-1 if none)
    int nxt_sib;      // Next sibling index (-1 if none)
    int move;         // Move that led to this node (-1 for root)
    int wins;         // Win count (from black's perspective, scaled by 2 for half-wins)
    int visits;       // Visit count
    bool fully_expanded;
    
    void init() {
        fa = fst_child = nxt_sib = move = -1;
        wins = visits = 0;
        fully_expanded = false;
    }
};

// ============================================================================
// MCTS Search
// ============================================================================
constexpr double MCTS_C = 2.0;           // Exploration constant
constexpr int ROLLOUT_MAX_DEPTH = 100;   // Max rollout depth

// Global node pool
extern Node nodes[MAX_NODES];
extern int node_count;

// Reset the node pool
void reset_nodes();

// Allocate a new node, returns node index
int alloc_node();

// Perform MCTS search and return best move
int getBest(Board& board, int iters);

// DFS function for MCTS
// Returns result from black's perspective: 1.0 = black win, 0.0 = white win, 0.5 = draw
double dfs(int node_id, int lst1, int lst2, Board& board);

// Rollout from current board state
double rollout(Board board, int depth_limit = ROLLOUT_MAX_DEPTH);

// Scan threats for a given position
// Returns the highest threat level move, populates move_list with all moves at that level
int scan_threats(const Board& board, int last_move, int side, 
                 int* move_list, int& move_count, bool is_self);

#endif // SEARCH_H

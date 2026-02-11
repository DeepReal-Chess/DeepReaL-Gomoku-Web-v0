#include "search.h"
#include <cstring>
#include <algorithm>
#include <random>

// Fast xorshift64* RNG for rollouts
static inline uint64_t xorshift64(uint64_t& state) {
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    return state * 0x2545F4914F6CDD1DULL;
}

// Forward declaration
static bool opponent_blocks_pattern(int player_mask, int opp_mask, int p);

// ============================================================================
// Global tables and node pool
// ============================================================================
int8_t threat_table[1 << 15][15];
int8_t winning_table[1 << 15][15];
Node nodes[MAX_NODES];
int node_count = 0;

// ============================================================================
// Threat table initialization
// 
// For a 15-bit mask representing a line, and a position p (0-14):
// - threat_table[mask][p] = threat level if playing at position p
// - winning_table[mask][p] = whether this move forces a win (for live3 patterns)
//
// Patterns (X = our pieces, . = empty, O = blocked/opponent/edge):
// Open4 (level 4 for self, level 3 for opponent):
//   OXXXX. -> play at pos 5 (the dot after 4 X's, with blocked left)
//   .XXXXO -> play at pos 0 (the dot before 4 X's, with blocked right)  
//   OXXX.X -> play at the gap
//   OXX.XX -> play at the gap
//   OX.XXX -> play at the gap
//   (and mirrored versions)
//
// Live3 (level 2 for self, level 1 for opponent):
//   .XXX.  -> play at either end (threat) or N/A for winning
//   .X.XX. -> play at gap (winning) or ends (threat only)
//   .XX.X. -> play at gap (winning) or ends (threat only)
// ============================================================================

// Helper: count consecutive bits starting at position p going right
static int count_consecutive_right(int mask, int p) {
    int cnt = 0;
    while (p < 15 && ((mask >> p) & 1)) {
        cnt++;
        p++;
    }
    return cnt;
}

// Helper: count consecutive bits starting at position p going left
static int count_consecutive_left(int mask, int p) {
    int cnt = 0;
    while (p >= 0 && ((mask >> p) & 1)) {
        cnt++;
        p--;
    }
    return cnt;
}

// Helper: check if position is empty (bit not set) and within bounds
static bool is_empty(int mask, int p) {
    if (p < 0 || p >= 15) return false;
    return ((mask >> p) & 1) == 0;
}

// Helper: check if position is blocked (out of bounds or has a piece)
static bool is_blocked(int mask, int p) {
    if (p < 0 || p >= 15) return true;  // Edge = blocked
    return ((mask >> p) & 1) == 1;
}

void init_threat_tables() {
    memset(threat_table, 0, sizeof(threat_table));
    memset(winning_table, 0, sizeof(winning_table));
    
    for (int mask = 0; mask < (1 << 15); mask++) {
        for (int p = 0; p < 15; p++) {
            // Skip if position p is already occupied
            if ((mask >> p) & 1) continue;
            
            int threat = THREAT_NONE;
            int winning = THREAT_NONE;
            
            // Check Open4 patterns (4 in a row with one gap or end)
            // Pattern: We need exactly 4 pieces that become 5 when we play at p
            
            // Case 1: p is at the end of 4 consecutive pieces
            // Check: XXXX. (4 pieces to the left of p, p is empty)
            {
                int left_cnt = count_consecutive_left(mask, p - 1);
                if (left_cnt >= 4) {
                    // Check if right side is open (for it to be "open four")
                    // Playing here wins, and left is blocked (edge or piece pattern)
                    // This is an Open4 - winning move
                    threat = std::max(threat, THREAT_OPEN4_WIN);
                    winning = std::max(winning, THREAT_OPEN4_WIN);
                }
            }
            
            // Case 2: .XXXX (4 pieces to the right of p)
            {
                int right_cnt = count_consecutive_right(mask, p + 1);
                if (right_cnt >= 4) {
                    threat = std::max(threat, THREAT_OPEN4_WIN);
                    winning = std::max(winning, THREAT_OPEN4_WIN);
                }
            }
            
            // Case 3: Gap in 4 - patterns like XXX.X, XX.XX, X.XXX
            // We need to find if placing at p creates 5 in a row
            {
                // Count pieces on both sides of p
                int left_cnt = count_consecutive_left(mask, p - 1);
                int right_cnt = count_consecutive_right(mask, p + 1);
                
                if (left_cnt + right_cnt >= 4) {
                    // Playing at p connects pieces to make 5+
                    // Check if at least one end is open for "open four"
                    int left_end = p - 1 - left_cnt;   // Position just left of the left group
                    int right_end = p + 1 + right_cnt; // Position just right of the right group
                    
                    // This is a winning move (makes 5 in a row)
                    threat = std::max(threat, THREAT_OPEN4_WIN);
                    winning = std::max(winning, THREAT_OPEN4_WIN);
                }
            }
            
            // Case 4: Open Four with gap not at ends
            // Patterns: OXXX.X, OXX.XX, OX.XXX (and mirrors)
            // One side blocked, gap in middle, playing at gap threatens win
            // But we need to be careful: the resulting 4-in-a-row needs one open end
            
            // Let's check for patterns where we have 3 consecutive + 1 with a gap
            // Pattern: XXX_X (3 left, gap at p, 1 right) or X_XXX (1 left, gap, 3 right)
            // or XX_XX (2 left, gap, 2 right)
            
            // XXX.X pattern: 3 to the left, 1 to the right
            if (count_consecutive_left(mask, p - 1) == 3 && 
                count_consecutive_right(mask, p + 1) == 1) {
                // After playing p, we have XXXXX - that's 5, so it's already caught above
                // But if it's blocked on left: OXXX.X -> playing at . makes OXXXXX
                // Let's check the "partial" case where total < 4
            }
            
            // Actually, the cases above already handle all 5-in-a-row completions.
            // Now let's handle Open4 THREAT (not winning but forcing):
            // Open4 means: 4 in a row with exactly one open end
            // If we play at p and create 4-in-a-row with one open end, opponent must block
            
            // For Open4 patterns like OXXXX. where playing at . creates blocked 4:
            // Wait, that's still a win (5 in a row). 
            
            // I think the key insight is:
            // - THREAT_OPEN4_WIN (level 4): Playing here makes 5 in a row = immediate win
            // - THREAT_OPEN4_THREAT is for opponent's patterns (handled during search)
            
            // Now handle Live3 patterns (both ends open)
            // .XXX. -> playing at either end creates .XXXX or XXXX.
            // .X.XX. -> playing at gap creates .XXXX. (live4), at ends creates XXXXX (if opponent blocks wrong)
            // .XX.X. -> same as above
            
            // Check for .XXX. pattern
            // Playing at p where XXX is to one side and both ends are open
            {
                // Pattern: .XXX. - p is at one of the dots
                // If p is the left dot: check XXX to the right, and dot after XXX
                int right_cnt = count_consecutive_right(mask, p + 1);
                if (right_cnt == 3) {
                    int after_right = p + 1 + right_cnt; // Position after the 3 X's
                    if (is_empty(mask, after_right)) {
                        // .XXX. pattern - p is the left dot
                        // Playing here is a threat (forces response) but not immediately winning
                        threat = std::max(threat, THREAT_LIVE3_WIN);
                        // This is NOT a winning move because opponent can block
                        // winning stays as-is
                    }
                }
                
                // If p is the right dot: check XXX to the left, and dot before XXX
                int left_cnt = count_consecutive_left(mask, p - 1);
                if (left_cnt == 3) {
                    int before_left = p - 1 - left_cnt; // Position before the 3 X's
                    if (is_empty(mask, before_left)) {
                        // .XXX. pattern - p is the right dot
                        threat = std::max(threat, THREAT_LIVE3_WIN);
                    }
                }
            }
            
            // Check for .X.XX. and .XX.X. patterns (live 3 with gap)
            // .X.XX. -> positions: dot, X, dot(gap), X, X, dot
            // If p is the gap: playing creates .XXXX. which is live4 (winning threat)
            // If p is an end dot: playing creates XX.XX. or .X.XXX which is still a threat
            
            // Pattern .X.XX. : 1 piece, gap, 2 pieces, with both outer ends open
            {
                // Check if p is the gap in .X.XX. pattern
                // Left side: one X, then open
                // Right side: two X's, then open
                if (count_consecutive_left(mask, p - 1) == 1 &&
                    count_consecutive_right(mask, p + 1) == 2) {
                    int left_end = p - 2;  // Before the single X
                    int right_end = p + 4; // After the two X's
                    if (is_empty(mask, left_end) && is_empty(mask, right_end)) {
                        // .X.XX. with p at the gap
                        threat = std::max(threat, THREAT_LIVE3_WIN);
                        winning = std::max(winning, THREAT_LIVE3_WIN);  // Gap move is "winning" for live3
                    }
                }
                
                // Check if p is the gap in .XX.X. pattern
                if (count_consecutive_left(mask, p - 1) == 2 &&
                    count_consecutive_right(mask, p + 1) == 1) {
                    int left_end = p - 3;  // Before the two X's
                    int right_end = p + 3; // After the single X
                    if (is_empty(mask, left_end) && is_empty(mask, right_end)) {
                        // .XX.X. with p at the gap
                        threat = std::max(threat, THREAT_LIVE3_WIN);
                        winning = std::max(winning, THREAT_LIVE3_WIN);
                    }
                }
            }
            
            // Check for end positions in .X.XX. and .XX.X. patterns (threat but not winning)
            {
                // .X.XX. -> left end dot is at position of the first dot
                // If there's X at p+1, gap at p+2, XX at p+3,p+4, and dot at p+5
                if (p + 5 < 15 &&
                    ((mask >> (p + 1)) & 1) == 1 &&  // X at p+1
                    ((mask >> (p + 2)) & 1) == 0 &&  // gap at p+2
                    ((mask >> (p + 3)) & 1) == 1 &&  // X at p+3
                    ((mask >> (p + 4)) & 1) == 1 &&  // X at p+4
                    is_empty(mask, p + 5)) {         // dot at p+5
                    threat = std::max(threat, THREAT_LIVE3_WIN);
                    // NOT winning - just threat
                }
                
                // .X.XX. -> right end dot
                if (p >= 5 &&
                    is_empty(mask, p - 5) &&          // dot at p-5
                    ((mask >> (p - 4)) & 1) == 1 &&  // X at p-4
                    ((mask >> (p - 3)) & 1) == 0 &&  // gap at p-3
                    ((mask >> (p - 2)) & 1) == 1 &&  // X at p-2
                    ((mask >> (p - 1)) & 1) == 1) {  // X at p-1
                    threat = std::max(threat, THREAT_LIVE3_WIN);
                }
                
                // .XX.X. -> left end dot
                if (p + 5 < 15 &&
                    ((mask >> (p + 1)) & 1) == 1 &&  // X at p+1
                    ((mask >> (p + 2)) & 1) == 1 &&  // X at p+2
                    ((mask >> (p + 3)) & 1) == 0 &&  // gap at p+3
                    ((mask >> (p + 4)) & 1) == 1 &&  // X at p+4
                    is_empty(mask, p + 5)) {         // dot at p+5
                    threat = std::max(threat, THREAT_LIVE3_WIN);
                }
                
                // .XX.X. -> right end dot
                if (p >= 5 &&
                    is_empty(mask, p - 5) &&          // dot at p-5
                    ((mask >> (p - 4)) & 1) == 1 &&  // X at p-4
                    ((mask >> (p - 3)) & 1) == 1 &&  // X at p-3
                    ((mask >> (p - 2)) & 1) == 0 &&  // gap at p-2
                    ((mask >> (p - 1)) & 1) == 1) {  // X at p-1
                    threat = std::max(threat, THREAT_LIVE3_WIN);
                }
            }
            
            threat_table[mask][p] = threat;
            winning_table[mask][p] = winning;
        }
    }
}

// ============================================================================
// Node pool management
// ============================================================================
void reset_nodes() {
    node_count = 0;
}

int alloc_node() {
    if (node_count >= MAX_NODES) {
        return -1;  // Out of nodes
    }
    int id = node_count++;
    nodes[id].init();
    return id;
}

// ============================================================================
// Rollout - random playout until terminal or max depth
// ============================================================================
double rollout(Board board, int depth_limit) {
    static uint64_t rng_state = 12345678901234567ULL;
    
    int depth = 0;
    while (!board.isTerminal() && depth < depth_limit) {
        // Use popbit() for fast random move selection
        lbit256 moves = board.nxt;
        int move_count = moves.popcount();
        if (move_count == 0) return 0.5;  // Draw
        
        int target = xorshift64(rng_state) % move_count;
        
        // Pop 'target' bits to reach the target-th move
        int move = -1;
        for (int i = 0; i <= target; i++) {
            move = moves.popbit();
        }
        
        if (move == -1) return 0.5;  // Safety fallback
        
        board.set(move);
        depth++;
    }
    
    if (board.isTerminal()) {
        return board.res;  // 1 = black win, 0 = white win
    }
    return 0.5;  // Draw (max depth reached)
}

// ============================================================================
// Scan ALL lines on the board for threats from player 'player'
// Used when we don't have a specific last_move to scan around (e.g., at root)
// ============================================================================
int scan_all_threats(const Board& board, int player,
                     int* move_list, int& move_count, bool is_self) {
    move_count = 0;
    int best_level = THREAT_NONE;
    int opp = 1 - player;
    
    // Temporary storage for moves at each level
    int temp_moves[4][64];  // [level-1][moves]
    int temp_counts[4] = {0, 0, 0, 0};
    
    // Scan all 15 rows
    for (int r = 0; r < 15; r++) {
        int row_player = board.row[player][r];
        int row_opp = board.row[opp][r];
        int row_combined = row_player | row_opp;
        
        for (int cc = 0; cc < 15; cc++) {
            int idx = r * 15 + cc;
            if ((row_combined >> cc) & 1) continue;  // Position occupied
            
            int t = threat_table[row_player][cc];
            int w = winning_table[row_player][cc];
            
            if ((t > 0 || w > 0) && opponent_blocks_pattern(row_player, row_opp, cc)) {
                t = THREAT_NONE;
                w = THREAT_NONE;
            }
            
            int level = std::max(t, w);
            
            if (!is_self && level > 0) {
                level = (level == THREAT_OPEN4_WIN) ? THREAT_OPEN4_THREAT : 
                        (level == THREAT_LIVE3_WIN) ? THREAT_LIVE3_THREAT : level;
            }
            
            if (level > 0) {
                int lvl_idx = level - 1;
                bool dup = false;
                for (int i = 0; i < temp_counts[lvl_idx]; i++) {
                    if (temp_moves[lvl_idx][i] == idx) { dup = true; break; }
                }
                if (!dup) {
                    temp_moves[lvl_idx][temp_counts[lvl_idx]++] = idx;
                    best_level = std::max(best_level, level);
                }
            }
        }
    }
    
    // Scan all 15 columns
    for (int c = 0; c < 15; c++) {
        int col_player = board.col[player][c];
        int col_opp = board.col[opp][c];
        int col_combined = col_player | col_opp;
        
        for (int rr = 0; rr < 15; rr++) {
            int idx = rr * 15 + c;
            if ((col_combined >> rr) & 1) continue;
            
            int t = threat_table[col_player][rr];
            int w = winning_table[col_player][rr];
            
            if ((t > 0 || w > 0) && opponent_blocks_pattern(col_player, col_opp, rr)) {
                t = THREAT_NONE;
                w = THREAT_NONE;
            }
            
            int level = std::max(t, w);
            
            if (!is_self && level > 0) {
                level = (level == THREAT_OPEN4_WIN) ? THREAT_OPEN4_THREAT : 
                        (level == THREAT_LIVE3_WIN) ? THREAT_LIVE3_THREAT : level;
            }
            
            if (level > 0) {
                int lvl_idx = level - 1;
                bool dup = false;
                for (int i = 0; i < temp_counts[lvl_idx]; i++) {
                    if (temp_moves[lvl_idx][i] == idx) { dup = true; break; }
                }
                if (!dup) {
                    temp_moves[lvl_idx][temp_counts[lvl_idx]++] = idx;
                    best_level = std::max(best_level, level);
                }
            }
        }
    }
    
    // Scan all 29 diagonal1 lines
    for (int d1_idx = 0; d1_idx < 29; d1_idx++) {
        int d1_player = board.diag1[player][d1_idx];
        int d1_opp = board.diag1[opp][d1_idx];
        int d1_combined = d1_player | d1_opp;
        
        int len = (d1_idx <= 14) ? (d1_idx + 1) : (29 - d1_idx);
        len = std::min(len, 15);
        
        int start_r = (d1_idx <= 14) ? 0 : (d1_idx - 14);
        int start_c = (d1_idx <= 14) ? (14 - d1_idx) : 0;
        
        for (int i = 0; i < len; i++) {
            int rr = start_r + i;
            int cc = start_c + i;
            if (rr < 0 || rr >= 15 || cc < 0 || cc >= 15) continue;
            int idx = rr * 15 + cc;
            if ((d1_combined >> i) & 1) continue;
            
            int t = threat_table[d1_player][i];
            int w = winning_table[d1_player][i];
            
            if ((t > 0 || w > 0) && opponent_blocks_pattern(d1_player, d1_opp, i)) {
                t = THREAT_NONE;
                w = THREAT_NONE;
            }
            
            int level = std::max(t, w);
            
            if (!is_self && level > 0) {
                level = (level == THREAT_OPEN4_WIN) ? THREAT_OPEN4_THREAT : 
                        (level == THREAT_LIVE3_WIN) ? THREAT_LIVE3_THREAT : level;
            }
            
            if (level > 0) {
                int lvl_idx = level - 1;
                bool dup = false;
                for (int j = 0; j < temp_counts[lvl_idx]; j++) {
                    if (temp_moves[lvl_idx][j] == idx) { dup = true; break; }
                }
                if (!dup) {
                    temp_moves[lvl_idx][temp_counts[lvl_idx]++] = idx;
                    best_level = std::max(best_level, level);
                }
            }
        }
    }
    
    // Scan all 29 diagonal2 lines
    for (int d2_idx = 0; d2_idx < 29; d2_idx++) {
        int d2_player = board.diag2[player][d2_idx];
        int d2_opp = board.diag2[opp][d2_idx];
        int d2_combined = d2_player | d2_opp;
        
        int len = (d2_idx <= 14) ? (d2_idx + 1) : (29 - d2_idx);
        len = std::min(len, 15);
        
        int start_r = (d2_idx <= 14) ? 0 : (d2_idx - 14);
        int start_c = (d2_idx <= 14) ? d2_idx : 14;
        
        for (int i = 0; i < len; i++) {
            int rr = start_r + i;
            int cc = start_c - i;
            if (rr < 0 || rr >= 15 || cc < 0 || cc >= 15) continue;
            int idx = rr * 15 + cc;
            if ((d2_combined >> i) & 1) continue;
            
            int t = threat_table[d2_player][i];
            int w = winning_table[d2_player][i];
            
            if ((t > 0 || w > 0) && opponent_blocks_pattern(d2_player, d2_opp, i)) {
                t = THREAT_NONE;
                w = THREAT_NONE;
            }
            
            int level = std::max(t, w);
            
            if (!is_self && level > 0) {
                level = (level == THREAT_OPEN4_WIN) ? THREAT_OPEN4_THREAT : 
                        (level == THREAT_LIVE3_WIN) ? THREAT_LIVE3_THREAT : level;
            }
            
            if (level > 0) {
                int lvl_idx = level - 1;
                bool dup = false;
                for (int j = 0; j < temp_counts[lvl_idx]; j++) {
                    if (temp_moves[lvl_idx][j] == idx) { dup = true; break; }
                }
                if (!dup) {
                    temp_moves[lvl_idx][temp_counts[lvl_idx]++] = idx;
                    best_level = std::max(best_level, level);
                }
            }
        }
    }
    
    // Collect moves at best level
    if (best_level > 0) {
        int lvl_idx = best_level - 1;
        for (int i = 0; i < temp_counts[lvl_idx]; i++) {
            move_list[move_count++] = temp_moves[lvl_idx][i];
        }
    }
    
    return best_level;
}

// ============================================================================
// Scan threats on lines passing through last_move
// Scans for threats from player 'player' (0=black, 1=white)
// is_self: true if scanning for current player's winning moves, false for opponent's threats
// Returns highest threat level found, populates move_list
//
// CORRECT APPROACH:
// 1. Use player-only mask for threat_table lookup (as table was designed)
// 2. Check that opponent doesn't occupy positions the pattern expects empty
// ============================================================================

// Check if opponent blocks any pattern positions in the 5-cell window around p
// A threat pattern requires certain positions to be empty (not occupied by opponent)
static bool opponent_blocks_pattern(int player_mask, int opp_mask, int p) {
    // The 5-cell window around p: positions p-4 to p+4 
    // For a valid threat at p, opponent cannot occupy positions that 
    // the pattern considers "empty" (where player doesn't have a piece)
    
    // Get the positions where player doesn't have pieces (potential "empty" in pattern)
    // If opponent occupies any of these in the relevant window, the pattern is blocked
    int window_start = std::max(0, p - 4);
    int window_end = std::min(14, p + 4);
    
    for (int i = window_start; i <= window_end; i++) {
        // Position i is "empty" in the pattern if player doesn't have a piece there
        bool player_has = (player_mask >> i) & 1;
        bool opp_has = (opp_mask >> i) & 1;
        
        // If opponent has a piece where the pattern expects empty, it's blocked
        if (!player_has && opp_has) {
            return true;  // Opponent blocks this pattern
        }
    }
    return false;  // Pattern not blocked
}

int scan_threats(const Board& board, int last_move, int player,
                 int* move_list, int& move_count, bool is_self) {
    move_count = 0;
    if (last_move < 0) return THREAT_NONE;
    
    int best_level = THREAT_NONE;
    int r = row_id[last_move];
    int c = col_id[last_move];
    int opp = 1 - player;
    
    // Player-only masks for threat_table lookup (as designed)
    int row_player = board.row[player][r];
    int col_player = board.col[player][c];
    int d1_idx = diag1_idx[last_move];
    int d1_player = board.diag1[player][d1_idx];
    int d2_idx = diag2_idx[last_move];
    int d2_player = board.diag2[player][d2_idx];
    
    // Opponent masks to check for blocking
    int row_opp = board.row[opp][r];
    int col_opp = board.col[opp][c];
    int d1_opp = board.diag1[opp][d1_idx];
    int d2_opp = board.diag2[opp][d2_idx];
    
    // Combined masks for occupancy check
    int row_combined = row_player | row_opp;
    int col_combined = col_player | col_opp;
    int d1_combined = d1_player | d1_opp;
    int d2_combined = d2_player | d2_opp;
    
    // Temporary storage for moves at each level
    int temp_moves[4][64];  // [level-1][moves]
    int temp_counts[4] = {0, 0, 0, 0};
    
    // Check row
    for (int cc = 0; cc < 15; cc++) {
        int idx = r * 15 + cc;
        if ((row_combined >> cc) & 1) continue;  // Position already occupied
        
        // Look up threat using player-only mask
        int t = threat_table[row_player][cc];
        int w = winning_table[row_player][cc];
        
        // Check if opponent blocks the pattern
        if ((t > 0 || w > 0) && opponent_blocks_pattern(row_player, row_opp, cc)) {
            t = THREAT_NONE;
            w = THREAT_NONE;
        }
        
        int level = std::max(t, w);
        
        // Adjust level for opponent's threats
        if (!is_self && level > 0) {
            level = (level == THREAT_OPEN4_WIN) ? THREAT_OPEN4_THREAT : 
                    (level == THREAT_LIVE3_WIN) ? THREAT_LIVE3_THREAT : level;
        }
        
        if (level > 0) {
            int lvl_idx = level - 1;
            temp_moves[lvl_idx][temp_counts[lvl_idx]++] = idx;
            best_level = std::max(best_level, level);
        }
    }
    
    // Check column
    for (int rr = 0; rr < 15; rr++) {
        int idx = rr * 15 + c;
        if ((col_combined >> rr) & 1) continue;
        
        int t = threat_table[col_player][rr];
        int w = winning_table[col_player][rr];
        
        if ((t > 0 || w > 0) && opponent_blocks_pattern(col_player, col_opp, rr)) {
            t = THREAT_NONE;
            w = THREAT_NONE;
        }
        
        int level = std::max(t, w);
        
        if (!is_self && level > 0) {
            level = (level == THREAT_OPEN4_WIN) ? THREAT_OPEN4_THREAT : 
                    (level == THREAT_LIVE3_WIN) ? THREAT_LIVE3_THREAT : level;
        }
        
        if (level > 0) {
            int lvl_idx = level - 1;
            bool dup = false;
            for (int i = 0; i < temp_counts[lvl_idx]; i++) {
                if (temp_moves[lvl_idx][i] == idx) { dup = true; break; }
            }
            if (!dup) {
                temp_moves[lvl_idx][temp_counts[lvl_idx]++] = idx;
                best_level = std::max(best_level, level);
            }
        }
    }
    
    // Check diagonal 1 (top-left to bottom-right)
    {
        int d1y = diag1_idy[last_move];
        int start_r = r - d1y;
        int start_c = c - d1y;
        int len = (d1_idx <= 14) ? (d1_idx + 1) : (29 - d1_idx);
        len = std::min(len, 15);
        
        for (int i = 0; i < len; i++) {
            int rr = start_r + i;
            int cc = start_c + i;
            if (rr < 0 || rr >= 15 || cc < 0 || cc >= 15) continue;
            int idx = rr * 15 + cc;
            if ((d1_combined >> i) & 1) continue;
            
            int t = threat_table[d1_player][i];
            int w = winning_table[d1_player][i];
            
            if ((t > 0 || w > 0) && opponent_blocks_pattern(d1_player, d1_opp, i)) {
                t = THREAT_NONE;
                w = THREAT_NONE;
            }
            
            int level = std::max(t, w);
            
            if (!is_self && level > 0) {
                level = (level == THREAT_OPEN4_WIN) ? THREAT_OPEN4_THREAT : 
                        (level == THREAT_LIVE3_WIN) ? THREAT_LIVE3_THREAT : level;
            }
            
            if (level > 0) {
                int lvl_idx = level - 1;
                bool dup = false;
                for (int j = 0; j < temp_counts[lvl_idx]; j++) {
                    if (temp_moves[lvl_idx][j] == idx) { dup = true; break; }
                }
                if (!dup) {
                    temp_moves[lvl_idx][temp_counts[lvl_idx]++] = idx;
                    best_level = std::max(best_level, level);
                }
            }
        }
    }
    
    // Check diagonal 2 (top-right to bottom-left)
    {
        int d2y = diag2_idy[last_move];
        int start_r = r - d2y;
        int start_c = c + d2y;
        int len = (d2_idx <= 14) ? (d2_idx + 1) : (29 - d2_idx);
        len = std::min(len, 15);
        
        for (int i = 0; i < len; i++) {
            int rr = start_r + i;
            int cc = start_c - i;
            if (rr < 0 || rr >= 15 || cc < 0 || cc >= 15) continue;
            int idx = rr * 15 + cc;
            if ((d2_combined >> i) & 1) continue;
            
            int t = threat_table[d2_player][i];
            int w = winning_table[d2_player][i];
            
            if ((t > 0 || w > 0) && opponent_blocks_pattern(d2_player, d2_opp, i)) {
                t = THREAT_NONE;
                w = THREAT_NONE;
            }
            
            int level = std::max(t, w);
            
            if (!is_self && level > 0) {
                level = (level == THREAT_OPEN4_WIN) ? THREAT_OPEN4_THREAT : 
                        (level == THREAT_LIVE3_WIN) ? THREAT_LIVE3_THREAT : level;
            }
            
            if (level > 0) {
                int lvl_idx = level - 1;
                bool dup = false;
                for (int j = 0; j < temp_counts[lvl_idx]; j++) {
                    if (temp_moves[lvl_idx][j] == idx) { dup = true; break; }
                }
                if (!dup) {
                    temp_moves[lvl_idx][temp_counts[lvl_idx]++] = idx;
                    best_level = std::max(best_level, level);
                }
            }
        }
    }
    
    // Collect moves at best level
    if (best_level > 0) {
        int lvl_idx = best_level - 1;
        for (int i = 0; i < temp_counts[lvl_idx]; i++) {
            move_list[move_count++] = temp_moves[lvl_idx][i];
        }
    }
    
    return best_level;
}

// ============================================================================
// UCB1 calculation
// ============================================================================
static inline double ucb(int wins, int visits, int parent_visits, int side) {
    if (visits == 0) return 1e18;  // Unvisited node has infinite priority
    
    // wins is from black's perspective
    // If it's white's turn (side=1), we want to minimize black's wins
    double win_rate = (double)wins / (2.0 * visits);  // Divide by 2 because wins are scaled
    if (side == 1) win_rate = 1.0 - win_rate;  // Flip for white
    
    return win_rate + MCTS_C * sqrt(log((double)parent_visits) / visits);
}

// ============================================================================
// DFS for MCTS
// ============================================================================
double dfs(int node_id, int lst1, int lst2, Board& board) {
    Node& node = nodes[node_id];
    
    // Terminal check
    if (board.isTerminal()) {
        double result = board.res;  // 1.0 for black win, 0.0 for white win
        node.wins += (int)(result * 2);  // Scale by 2 for half-wins
        node.visits++;
        return result;
    }
    
    int side = board.side();  // 0 = black to move, 1 = white to move
    
    // If fully expanded, select best child
    if (node.fully_expanded) {
        int best_child = -1;
        double best_ucb = -1e18;
        
        for (int child = node.fst_child; child != -1; child = nodes[child].nxt_sib) {
            double u = ucb(nodes[child].wins, nodes[child].visits, node.visits, side);
            if (u > best_ucb) {
                best_ucb = u;
                best_child = child;
            }
        }
        
        if (best_child == -1) {
            // No children - shouldn't happen if fully expanded
            return 0.5;
        }
        
        board.set(nodes[best_child].move);
        double result = dfs(best_child, lst2, nodes[best_child].move, board);
        
        node.wins += (int)(result * 2);
        node.visits++;
        return result;
    }
    
    // First visit - check for threats
    if (node.visits == 0) {
        int move_list[64];
        int move_count = 0;
        int best_threat = THREAT_NONE;
        
        // Scan for our winning moves
        // If lst2 == -1 (at root), scan entire board
        int self_moves[64], self_count = 0;
        int self_threat;
        if (lst2 < 0) {
            self_threat = scan_all_threats(board, side, self_moves, self_count, true);
        } else {
            self_threat = scan_threats(board, lst2, side, self_moves, self_count, true);
        }
        
        // Scan for opponent's threats
        // If lst1 == -1 (opponent hasn't moved), scan entire board for their threats
        int opp_moves[64], opp_count = 0;
        int opp_threat;
        if (lst1 < 0) {
            opp_threat = scan_all_threats(board, 1 - side, opp_moves, opp_count, false);
        } else {
            opp_threat = scan_threats(board, lst1, 1 - side, opp_moves, opp_count, false);
        }
        
        // Combine: prioritize by level
        if (self_threat >= opp_threat && self_threat > THREAT_NONE) {
            best_threat = self_threat;
            for (int i = 0; i < self_count; i++) {
                move_list[move_count++] = self_moves[i];
            }
        } else if (opp_threat > THREAT_NONE) {
            best_threat = opp_threat;
            for (int i = 0; i < opp_count; i++) {
                move_list[move_count++] = opp_moves[i];
            }
        }
        
        if (best_threat > THREAT_NONE && move_count > 0) {
            // Expand only threat moves, mark fully expanded
            for (int i = 0; i < move_count; i++) {
                int child_id = alloc_node();
                if (child_id == -1) break;
                
                nodes[child_id].fa = node_id;
                nodes[child_id].move = move_list[i];
                nodes[child_id].nxt_sib = node.fst_child;
                node.fst_child = child_id;
            }
            node.fully_expanded = true;
            
            // Now select and recurse
            int child = node.fst_child;
            board.set(nodes[child].move);
            double result = dfs(child, lst2, nodes[child].move, board);
            
            node.wins += (int)(result * 2);
            node.visits++;
            return result;
        }
        
        // No threats - do rollout
        double result = rollout(board);
        node.wins += (int)(result * 2);
        node.visits++;
        return result;
    }
    
    // Node has been visited but not fully expanded
    // Check iterative expansion: if C * sqrt(ln(visits)) > max UCB of children, expand
    double expand_threshold = MCTS_C * sqrt(log((double)node.visits));
    
    int best_child = -1;
    double best_ucb = -1e18;
    
    for (int child = node.fst_child; child != -1; child = nodes[child].nxt_sib) {
        double u = ucb(nodes[child].wins, nodes[child].visits, node.visits, side);
        if (u > best_ucb) {
            best_ucb = u;
            best_child = child;
        }
    }
    
    // If we should expand a new node
    if (best_child == -1 || expand_threshold > best_ucb) {
        // Find an unexpanded move
        lbit256 expanded_moves;
        for (int child = node.fst_child; child != -1; child = nodes[child].nxt_sib) {
            expanded_moves.set(nodes[child].move);
        }
        
        lbit256 unexpanded = board.nxt & ~expanded_moves;
        
        if (!unexpanded.empty()) {
            int new_move = unexpanded.popbit();
            
            int child_id = alloc_node();
            if (child_id != -1) {
                nodes[child_id].fa = node_id;
                nodes[child_id].move = new_move;
                nodes[child_id].nxt_sib = node.fst_child;
                node.fst_child = child_id;
                
                board.set(new_move);
                double result = rollout(board);
                
                nodes[child_id].wins += (int)(result * 2);
                nodes[child_id].visits++;
                node.wins += (int)(result * 2);
                node.visits++;
                return result;
            }
        } else {
            // All moves expanded
            node.fully_expanded = true;
        }
    }
    
    // Select best child and recurse
    if (best_child != -1) {
        board.set(nodes[best_child].move);
        double result = dfs(best_child, lst2, nodes[best_child].move, board);
        
        node.wins += (int)(result * 2);
        node.visits++;
        return result;
    }
    
    // Fallback - shouldn't reach here
    return 0.5;
}

// ============================================================================
// getBest - main MCTS entry point
// ============================================================================
int getBest(Board& board, int iters) {
    reset_nodes();
    
    int root = alloc_node();
    
    // Handle first move - play center
    if (board.cnt == 0) {
        return 7 * 15 + 7;  // Center
    }
    
    // Handle case with only one legal move
    if (board.nxt.popcount() == 1) {
        lbit256 temp = board.nxt;
        return temp.popbit();
    }
    
    // Determine lst1 (opponent's last move) for threat scanning
    // At root, opponent just played, so board.last_move is opponent's move
    int opponent_last = board.last_move;
    
    for (int i = 0; i < iters; i++) {
        Board copy = board;
        dfs(root, opponent_last, -1, copy);
    }
    
    // Select move with most visits
    int best_move = -1;
    int best_visits = -1;
    
    for (int child = nodes[root].fst_child; child != -1; child = nodes[child].nxt_sib) {
        if (nodes[child].visits > best_visits) {
            best_visits = nodes[child].visits;
            best_move = nodes[child].move;
        }
    }
    
    return best_move;
}

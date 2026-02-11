// ============================================================================
// DeepReaL Gomoku Engine v0 - WebAssembly Entry Point
// Emscripten-compatible wrapper using text-based UCI protocol via stdin/stdout
// ============================================================================

#include "../DeepReaL-Gomoku-Engine-v0-main/src/board.h"
#include "../DeepReaL-Gomoku-Engine-v0-main/src/search.h"
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <string>
#include <sstream>

static Board g_board;
static bool g_initialized = false;

// Initialize the engine (precomputed tables + board)
std::string engine_init() {
    init_precompute();
    init_threat_tables();
    g_board.init();
    g_initialized = true;
    return "ready";
}

// Make a move at the given index (0-224)
std::string engine_update(int index) {
    if (!g_initialized) return "error: not initialized";
    if (index < 0 || index >= 225) return "error: invalid index";
    if (g_board.res != -1) return "error: game already ended";
    
    // Check if position is already occupied
    if (g_board.b[0].get(index) || g_board.b[1].get(index)) {
        return "error: position occupied";
    }
    
    g_board.set(index);
    
    // Check for terminal state after move
    if (g_board.res == 1) return "ok win black";
    if (g_board.res == 0) return "ok win white";
    
    // Check for draw (board full)
    if (g_board.cnt >= 225) return "ok draw";
    
    return "ok";
}

// Run MCTS and return best move
std::string engine_go(int iters) {
    if (!g_initialized) return "error: not initialized";
    if (g_board.res != -1) return "error: game already ended";
    
    if (iters <= 0) iters = 10000;
    
    int best = getBest(g_board, iters);
    return "bestmove " + std::to_string(best);
}

// Get board state as a string
std::string engine_get_state() {
    if (!g_initialized) return "error: not initialized";
    
    std::ostringstream oss;
    oss << "state ";
    oss << "cnt=" << g_board.cnt << " ";
    oss << "side=" << g_board.side() << " ";
    oss << "res=" << g_board.res << " ";
    oss << "last=" << g_board.last_move << " ";
    
    // Board cells: 0=empty, 1=black, 2=white
    oss << "board=";
    for (int i = 0; i < 225; i++) {
        if (g_board.b[0].get(i)) oss << "1";
        else if (g_board.b[1].get(i)) oss << "2";
        else oss << "0";
    }
    
    return oss.str();
}

// Reset the board
std::string engine_reset() {
    if (!g_initialized) {
        return engine_init();
    }
    g_board.init();
    return "ready";
}

// Process a UCI-like command string
std::string engine_command(const std::string& cmd) {
    std::istringstream iss(cmd);
    std::string token;
    iss >> token;
    
    if (token == "init") {
        return engine_init();
    } else if (token == "reset") {
        return engine_reset();
    } else if (token == "update") {
        int index;
        if (iss >> index) {
            return engine_update(index);
        }
        return "error: missing index";
    } else if (token == "go") {
        int iters = 10000;
        iss >> iters;
        return engine_go(iters);
    } else if (token == "state") {
        return engine_get_state();
    } else if (token == "quit") {
        return "bye";
    }
    
    return "error: unknown command '" + token + "'";
}

EMSCRIPTEN_BINDINGS(gomoku_engine) {
    emscripten::function("engineCommand", &engine_command);
    emscripten::function("engineInit", &engine_init);
    emscripten::function("engineUpdate", &engine_update);
    emscripten::function("engineGo", &engine_go);
    emscripten::function("engineGetState", &engine_get_state);
    emscripten::function("engineReset", &engine_reset);
}

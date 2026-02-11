// ============================================================================
// DeepReaL Gomoku Engine v0 - Main Entry Point
// ============================================================================

#include "board.h"
#include "search.h"
#include "uci.h"
#include <iostream>

int main() {
    // Initialize precomputed tables
    init_precompute();
    init_threat_tables();
    
    // Enter UCI loop
    uci_loop();
    
    return 0;
}

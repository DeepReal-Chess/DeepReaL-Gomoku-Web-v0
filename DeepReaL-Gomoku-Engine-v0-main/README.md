# DeepReaL Gomoku Engine v0

A high-performance Gomoku (Five-in-a-Row) engine written in C++17, featuring bitboard representation and Monte Carlo Tree Search (MCTS).

## Features

- **Bitboard Representation**: 256-bit board (`lbit256`) for fast operations
- **MCTS Search**: UCB1-based tree search with threat detection
- **Threat Detection**: Pattern-based detection for Open4, Live3 patterns
- **UCI Interface**: Standard protocol for external GUI integration
- **Performance**: ~100K MCTS iterations/second, ~10ns per board update

## Building

```bash
make            # Build all targets
make gomoku     # Build UCI engine only
make demo       # Build demo program only
make test       # Build test suite only
make clean      # Clean build artifacts
```

Requires: C++17 compatible compiler (tested with g++/clang++)

## Usage

### UCI Engine
```bash
./gomoku
```
Supports UCI commands: `uci`, `isready`, `ucinewgame`, `position`, `go`, `d`, `quit`

### Demo
```bash
./demo
```
Interactive demo with three modes:
1. **Bot vs Bot**: Watch the engine play against itself
2. **Human vs Bot (Black)**: Play as Black (X), move first
3. **Human vs Bot (White)**: Play as White (O), move second

Move input formats: `row,col` or `(row,col)` or `row col` (0-indexed, 0-14)

### Tests
```bash
./test
```
Runs all unit tests including:
- Board representation correctness
- Win detection in all directions
- Next-move generation
- Threat table patterns
- Search quality tests
- Gap-blocked pattern validation

## Architecture

### Board Representation
- `lbit256`: 4×64-bit integers for 225-cell (15×15) board
- Per-player bitboards: `b[0]` (Black), `b[1]` (White)
- Line masks: `row[2][15]`, `col[2][15]`, `diag1[2][29]`, `diag2[2][29]`
- Incremental `nxt` bitboard: Chebyshev-2 neighborhood of all pieces

### Search
- MCTS with UCB1 (C=2.0)
- Threat-based move ordering at root
- Pattern tables: `threat_table[1<<15][15]`, `winning_table[1<<15][15]`
- Threat validation: Correctly handles opponent pieces blocking patterns

### Threat Levels
- `THREAT_OPEN4_WIN` (4): Completes 5-in-a-row
- `THREAT_OPEN4_THREAT` (3): Opponent's Open4
- `THREAT_LIVE3_WIN` (2): Creates live-4 pattern
- `THREAT_LIVE3_THREAT` (1): Opponent's Live3

## Files

```
├── Makefile              # Build configuration
├── README.md             # This file
├── src/
│   ├── board.h           # lbit256 and Board structs
│   ├── board.cpp         # Board implementation
│   ├── precompute.cpp    # Precomputed tables
│   ├── search.h          # Search interface
│   ├── search.cpp        # MCTS and threat detection
│   ├── uci.h             # UCI interface
│   ├── uci.cpp           # UCI implementation
│   ├── main.cpp          # UCI entry point
│   ├── demo.cpp          # Interactive demo
│   └── test.cpp          # Test suite
└── obj/                  # Build artifacts (generated)
```

## Performance

- Board `set()`: ~9ns
- `popbit()`: ~1ns  
- MCTS: ~125K iterations/sec
- Typical thinking time: 3 seconds (~375K iterations)

## Design Notes

### Threat Scanning
The engine uses precomputed threat tables indexed by 15-bit line masks. Key insight: tables are built for single-player patterns, so we lookup with player-only masks then validate that opponent pieces don't block the pattern's required empty positions.

### Gap Validation
Patterns like `.X.XX.` require the gap position to be truly empty. The `opponent_blocks_pattern()` function checks that no opponent pieces occupy any position in the 9-cell window that the pattern expects to be empty.

### MCTS Strategy
- First visit: Scan for threats, expand only threat moves if found
- Subsequent visits: Progressive widening based on UCB threshold
- Rollout: Random playout with fast xorshift64 RNG

## License

MIT License

## Author

DeepReaL Project
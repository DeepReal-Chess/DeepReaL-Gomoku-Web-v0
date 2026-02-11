# DeepReaL Gomoku Web

A fully client-side Gomoku (Five-in-a-Row) game that runs the DeepReaL MCTS engine directly in the browser via WebAssembly. No servers, no APIs — just pure WASM performance.

![Gomoku Web](https://img.shields.io/badge/Engine-MCTS%20%2B%20UCB1-blue)
![WASM](https://img.shields.io/badge/Runtime-WebAssembly-purple)
![License](https://img.shields.io/badge/License-MIT-green)

## Features

- **15×15 Gomoku board** with click-to-place stones
- **MCTS AI opponent** (Monte Carlo Tree Search with UCB1 + threat detection)
- **Runs entirely in-browser** — compiled to WebAssembly via Emscripten
- **Web Worker** — AI computation runs off the main thread (no UI freezing)
- **Configurable AI strength** — 0.5s, 1s, 5s, or 30s think time
- **Play as Black or White**
- **Win detection** with highlighted winning line
- **Neo-futuristic dark UI** with glassmorphism design
- **GitHub Pages compatible** — static files only

## Architecture

```
GomokuWeb v0/
├── DeepReaL-Gomoku-Engine-v0-main/   # C++ engine (unchanged)
│   └── src/
│       ├── board.{h,cpp}             # Bitboard representation
│       ├── precompute.cpp            # Precomputed tables
│       ├── search.{h,cpp}            # MCTS + threat detection
│       └── uci.{h,cpp}              # UCI interface
├── emsdk/                            # Emscripten SDK (local, gitignored)
├── wasm/
│   ├── main_wasm.cpp                 # WASM entry point (Emscripten bindings)
│   └── build.sh                      # Build script
├── web/
│   ├── index.html                    # Main page
│   ├── css/style.css                 # Neo-futuristic dark UI
│   ├── js/
│   │   ├── app.js                    # Application controller
│   │   ├── game.js                   # Game state manager
│   │   ├── ui.js                     # Canvas board renderer
│   │   ├── engine-interface.js       # Engine ↔ JS bridge
│   │   └── engine-worker.js          # Web Worker (loads WASM)
│   └── wasm/                         # Built WASM output (generated)
│       ├── gomoku_engine.js
│       └── gomoku_engine.wasm
├── Makefile                          # Build configuration
├── serve.py                          # No-cache dev server
└── README.md
```

### Data Flow

```
User Click → app.js → engine-interface.js → [postMessage] → engine-worker.js → WASM Module
                                                                                    ↓
UI Canvas ← app.js ← engine-interface.js ← [postMessage] ← engine-worker.js ← WASM Result
```

## Prerequisites

- **Emscripten SDK** (emsdk) — for compiling C++ to WebAssembly
- **Python 3** — for local development server (or any static HTTP server)
- **Modern browser** — Chrome, Firefox, Safari, or Edge

### Install Emscripten

The Emscripten SDK is included locally in this repo (gitignored). To set it up:

```bash
# Clone emsdk into the project root (if not already present)
git clone https://github.com/emscripten-core/emsdk.git emsdk

# Install and activate the latest SDK
cd emsdk
./emsdk install latest
./emsdk activate latest
cd ..
```

## Building

### Quick Build

```bash
# Activate emsdk for current shell
source emsdk/emsdk_env.sh

# Build WASM from project root
make wasm
```

### Using the build script

```bash
cd wasm
chmod +x build.sh
./build.sh
```

### Production Build (optimized)

```bash
make release
```

This produces `web/wasm/gomoku_engine.js` and `web/wasm/gomoku_engine.wasm`.

## Running Locally

```bash
# After building WASM, start a local server
make serve

# Or manually:
cd web
python3 -m http.server 8080
```

Then open **http://localhost:8080** in your browser.

## Deploying to GitHub Pages

1. **Build the WASM module** (see above)

2. **Push the `web/` directory** to GitHub Pages:
   ```bash
   # Option A: Deploy web/ folder directly
   # In your repo settings → Pages → Source → select branch + /web folder
   
   # Option B: Use gh-pages branch
   git subtree push --prefix web origin gh-pages
   ```

3. **Configure GitHub Pages:**
   - Go to Repository → Settings → Pages
   - Source: Deploy from branch
   - Branch: `gh-pages` (root) or `main` (`/web` folder)

4. Your site will be live at `https://<username>.github.io/<repo>/`

### Important: WASM MIME Type

GitHub Pages serves `.wasm` files with the correct `application/wasm` MIME type by default. No extra configuration needed.

## Engine Interface (WASM API)

The WASM module exposes these functions via Emscripten bindings:

| Function | Arguments | Returns | Description |
|---|---|---|---|
| `engineInit()` | — | `"ready"` | Initialize engine tables and board |
| `engineUpdate(index)` | `int 0-224` | `"ok"` / `"ok win black"` / `"ok win white"` | Play a move |
| `engineGo(iters)` | `int` | `"bestmove <index>"` | Run MCTS search |
| `engineReset()` | — | `"ready"` | Reset board to initial state |
| `engineGetState()` | — | State string | Get full board state |
| `engineCommand(cmd)` | `string` | Response string | Send raw UCI command |

**Board indexing:** `index = row * 15 + col` (0-indexed, row-major)

## Movetime → Iterations Mapping

| Time | Iterations | Strength |
|------|-----------|----------|
| 0.5s | 50,000 | Casual |
| 1s | 100,000 | Normal |
| 5s | 500,000 | Strong |
| 30s | 3,000,000 | Maximum |

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `N` | New Game |

## Browser Compatibility

| Browser | Supported | Notes |
|---------|-----------|-------|
| Chrome 57+ | ✅ | Full support |
| Firefox 53+ | ✅ | Full support |
| Safari 11+ | ✅ | Full support |
| Edge 16+ | ✅ | Full support |

## Tech Stack

- **C++17** — Engine core
- **Emscripten** — C++ → WebAssembly compiler
- **Vanilla JavaScript (ES Modules)** — No framework dependencies
- **HTML5 Canvas** — Board rendering
- **Web Workers** — Off-thread AI computation
- **CSS3** — Glassmorphism + dark mode UI

## License

MIT

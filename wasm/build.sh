#!/bin/bash
# ============================================================================
# Build script for DeepReaL Gomoku WASM module
# Requires: Emscripten SDK (emsdk) installed and activated
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
ENGINE_SRC="$PROJECT_ROOT/DeepReaL-Gomoku-Engine-v0-main/src"
WASM_SRC="$SCRIPT_DIR/main_wasm.cpp"
OUTPUT_DIR="$PROJECT_ROOT/web/wasm"

echo "=== DeepReaL Gomoku WASM Build ==="
echo "Engine source: $ENGINE_SRC"
echo "WASM source:   $WASM_SRC"
echo "Output:        $OUTPUT_DIR"

# Check for emcc
if ! command -v emcc &> /dev/null; then
    echo "ERROR: emcc not found. Please install and activate Emscripten SDK."
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk && ./emsdk install latest && ./emsdk activate latest"
    echo "  source emsdk_env.sh"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Compile
echo "Compiling..."
emcc \
    -std=c++17 \
    -O2 \
    -s WASM=1 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME="'GomokuEngine'" \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s INITIAL_MEMORY=536870912 \
    -s MAXIMUM_MEMORY=1073741824 \
    -s NO_EXIT_RUNTIME=1 \
    -s ENVIRONMENT='worker' \
    --bind \
    "$WASM_SRC" \
    "$ENGINE_SRC/board.cpp" \
    "$ENGINE_SRC/precompute.cpp" \
    "$ENGINE_SRC/search.cpp" \
    -o "$OUTPUT_DIR/gomoku_engine.js"

echo "=== Build complete ==="
echo "Output files:"
echo "  $OUTPUT_DIR/gomoku_engine.js"
echo "  $OUTPUT_DIR/gomoku_engine.wasm"
ls -la "$OUTPUT_DIR/gomoku_engine."*

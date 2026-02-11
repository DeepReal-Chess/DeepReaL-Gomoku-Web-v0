# ============================================================================
# DeepReaL Gomoku Web - Build Configuration
# ============================================================================

# Emscripten compiler (local emsdk)
EMSDK_DIR = emsdk
EMCC = emcc

# Paths
ENGINE_SRC = DeepReaL-Gomoku-Engine-v0-main/src
WASM_SRC = wasm/main_wasm.cpp
OUTPUT_DIR = web/wasm

# Engine source files
ENGINE_SRCS = $(ENGINE_SRC)/board.cpp $(ENGINE_SRC)/precompute.cpp $(ENGINE_SRC)/search.cpp

# Emscripten flags
EMFLAGS = -std=c++17 \
          -O2 \
          -s WASM=1 \
          -s MODULARIZE=1 \
          -s "EXPORT_NAME='GomokuEngine'" \
          -s ALLOW_MEMORY_GROWTH=1 \
          -s INITIAL_MEMORY=536870912 \
          -s MAXIMUM_MEMORY=1073741824 \
          -s NO_EXIT_RUNTIME=1 \
          -s "ENVIRONMENT='worker'" \
          --bind

# Default target
all: wasm

# Build WASM module
wasm: $(OUTPUT_DIR)/gomoku_engine.js

$(OUTPUT_DIR)/gomoku_engine.js: $(WASM_SRC) $(ENGINE_SRCS) | $(OUTPUT_DIR)
	@echo "=== Building Gomoku WASM module ==="
	$(EMCC) $(EMFLAGS) $(WASM_SRC) $(ENGINE_SRCS) -o $@
	@echo "=== Build complete ==="
	@ls -la $(OUTPUT_DIR)/gomoku_engine.*

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

# Clean build artifacts
clean:
	rm -rf $(OUTPUT_DIR)

# Serve locally for testing (no-cache headers for development)
serve:
	@echo "=== Starting local server on http://localhost:8080 ==="
	python3 serve.py

# Build optimized for production
release: EMFLAGS += -O3 -s ASSERTIONS=0 --closure 1
release: clean wasm

.PHONY: all wasm clean serve release

# Project Outline: Gomoku Web

You are an autonomous coding AI.

Your task is to build a fully functional GitHub Pages website that allows users to play Gomoku against my existing engine (`DeepReaL-Gomoku-Engine-v0-main`).


## GOAL

Create a static website (GitHub Pages compatible) where:
- A human can play Gomoku in the browser
- The AI opponent uses my existing Gomoku engine logic
- The engine runs locally in the browser (NO server)
- The site is performant, cleanly architected, and extensible

## ENGINE CONTEXT

I already have a high-performance Gomoku engine written in C++ with:
- Bitboard / custom board representation
- MCTS move selection
- UCI (Access `README.md` for details)

## Design

- Neo-Futuristic Soft UI (Dark Mode, Glass-Inspired)
- Dark Gradient Minimalism
- Modern Gomoku Board Design

## MANDATORY TECHNICAL CONSTRAINTS

- Must work on GitHub Pages (static hosting only)
- NO backend servers
- NO cloud APIs
- Engine must run via WebAssembly (WASM)
- Use Emscripten to compile C++ → WASM
- JS communicates with the engine via a clean interface
- Must support modern desktop browsers (Chrome / Firefox / Safari)

## FUNCTIONAL REQUIREMENTS

1. Gomoku board UI (15x15)
2. Click-to-place stones
3. Legal move enforcement
4. Clear turn system (Human vs AI)
5. AI responds after human move
6. Detect win / loss / draw
7. Restart game
8. Visual highlight of last move
9. Set movetime for AI (0.5s, 1s, 5s, 30s)

## NON-FUNCTIONAL REQUIREMENTS

- Engine computation must not freeze UI
- Clean separation between:
  - UI
  - Game state
  - Engine interface


## TECH STACK

- Vanilla JS
- HTML5 Canvas or SVG for board rendering
- Web Worker for AI computation
- WASM module loading strategy
- Minimal dependencies

## DELIVERABLES

1. Full repository structure
2. Build instructions (local + GitHub Pages)
3. Emscripten build commands
4. Engine ↔ JS interface definition
5. UI implementation
6. README with usage instructions
7. Deployed GitHub Pages URL instructions

## AUTONOMY RULES

- Do NOT simplify the engine logic
- If something is ambiguous, ask me.
- Optimize for clarity, correctness, and future extensibility
- Think like this is an open-source project with real users

Proceed step-by-step:
1. Architecture plan
2. WASM integration plan
3. UI design
4. Implementation
5. Deployment instructions

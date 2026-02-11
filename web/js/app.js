// ============================================================================
// DeepReaL Gomoku Web - Main Application Controller
// Ties together Engine, Game State, and UI
// ============================================================================

import { GameState, SIDE_BLACK, SIDE_WHITE } from './game.js';
import { BoardRenderer } from './ui.js';
import { EngineInterface } from './engine-interface.js';

class GomokuApp {
    constructor() {
        this.game = new GameState();
        this.engine = new EngineInterface();
        this.renderer = null;
        this.animLoop = null;
        
        // DOM elements
        this.statusEl = document.getElementById('status-text');
        this.moveCountEl = document.getElementById('move-count');
        this.thinkingEl = document.getElementById('thinking-indicator');
        this.resultOverlay = document.getElementById('result-overlay');
        this.resultText = document.getElementById('result-text');
        this.movetimeSelect = document.getElementById('movetime-select');
        this.sideSelect = document.getElementById('side-select');
        this.newGameBtn = document.getElementById('new-game-btn');
        this.canvas = document.getElementById('board-canvas');
    }

    /**
     * Initialize the application
     */
    async init() {
        // Setup renderer
        this.renderer = new BoardRenderer(this.canvas);
        this.renderer.onCellClick = (row, col) => this.handleCellClick(row, col);
        
        // Bind UI controls
        this._bindControls();
        
        // Start render loop
        this._startRenderLoop();
        
        // Initialize engine
        this._setStatus('Loading engine...');
        try {
            await this.engine.init();
            this._setStatus('Engine ready. Your turn!');
            this._checkAITurn();
        } catch (err) {
            this._setStatus('Engine failed to load: ' + err.message);
            console.error('Engine init error:', err);
        }
    }

    /**
     * Bind UI control events
     */
    _bindControls() {
        // New game button
        this.newGameBtn.addEventListener('click', () => this.newGame());
        
        // Movetime selector
        this.movetimeSelect.addEventListener('change', (e) => {
            this.game.movetime = parseFloat(e.target.value);
        });
        
        // Side selector
        this.sideSelect.addEventListener('change', (e) => {
            // Will apply on next new game
        });
        
        // Keyboard shortcuts
        document.addEventListener('keydown', (e) => {
            if (e.key === 'n' || e.key === 'N') {
                this.newGame();
            }
        });
        
        // Resize handler
        window.addEventListener('resize', () => {
            this.renderer.resize();
        });
    }

    /**
     * Start new game
     */
    async newGame() {
        // Hide result overlay
        this.resultOverlay.classList.remove('visible');
        
        // Reset game state
        this.game.reset();
        this.game.humanSide = parseInt(this.sideSelect.value);
        this.game.movetime = parseFloat(this.movetimeSelect.value);
        
        // Clear renderer state
        this.renderer.clearWinLine();
        
        // Reset engine
        try {
            await this.engine.reset();
            this._setStatus('Your turn!');
            this._updateMoveCount();
            this._checkAITurn();
        } catch (err) {
            this._setStatus('Error: ' + err.message);
        }
    }

    /**
     * Handle cell click from the board
     */
    async handleCellClick(row, col) {
        const index = GameState.toIndex(row, col);
        
        // Validate move
        if (!this.game.isHumanTurn()) return;
        if (!this.game.isLegalMove(index)) return;
        
        // Play human move
        await this._playMove(index);
        
        // Check if game is over
        if (this.game.isGameOver) {
            this._handleGameOver();
            return;
        }
        
        // Trigger AI response
        this._aiMove();
    }

    /**
     * Play a move (both human and AI)
     */
    async _playMove(index) {
        try {
            const response = await this.engine.update(index);
            this.game.recordMove(index);
            this.game.updateResult(response);
            this._updateMoveCount();
        } catch (err) {
            this._setStatus('Error: ' + err.message);
        }
    }

    /**
     * Request AI move from engine
     */
    async _aiMove() {
        if (this.game.isGameOver) return;
        
        this.game.isThinking = true;
        this._setStatus('AI is thinking...');
        this.thinkingEl.classList.add('visible');
        
        try {
            const iters = this.game.getIterations();
            const bestMove = await this.engine.go(iters);
            
            if (bestMove >= 0 && bestMove < 225) {
                await this._playMove(bestMove);
                
                if (this.game.isGameOver) {
                    this._handleGameOver();
                } else {
                    this._setStatus('Your turn');
                }
            } else {
                this._setStatus('AI returned invalid move');
            }
        } catch (err) {
            this._setStatus('AI error: ' + err.message);
        } finally {
            this.game.isThinking = false;
            this.thinkingEl.classList.remove('visible');
        }
    }

    /**
     * Check if AI should move (e.g., human plays white)
     */
    _checkAITurn() {
        if (this.game.isAITurn()) {
            this._aiMove();
        }
    }

    /**
     * Handle game over
     */
    _handleGameOver() {
        const line = this.game.getWinningLine();
        if (line) {
            this.renderer.setWinLine(line);
        }
        
        const text = this.game.getResultText();
        this._setStatus(text);
        
        // Show result overlay after a short delay
        setTimeout(() => {
            this.resultText.textContent = text;
            this.resultOverlay.classList.add('visible');
        }, 800);
    }

    /**
     * Update status text
     */
    _setStatus(text) {
        if (this.statusEl) {
            this.statusEl.textContent = text;
        }
    }

    /**
     * Update move count display
     */
    _updateMoveCount() {
        if (this.moveCountEl) {
            this.moveCountEl.textContent = `Move ${this.game.moveCount}`;
        }
    }

    /**
     * Render loop (requestAnimationFrame)
     */
    _startRenderLoop() {
        const loop = () => {
            this.renderer.render(this.game);
            this.animLoop = requestAnimationFrame(loop);
        };
        loop();
    }

    /**
     * Cleanup
     */
    destroy() {
        if (this.animLoop) cancelAnimationFrame(this.animLoop);
        this.engine.destroy();
    }
}

// ============================================================================
// Boot
// ============================================================================
window.addEventListener('DOMContentLoaded', () => {
    const app = new GomokuApp();
    app.init();
    
    // Expose for debugging
    window.__gomokuApp = app;
});

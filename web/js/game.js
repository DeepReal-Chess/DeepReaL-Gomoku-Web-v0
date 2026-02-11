// ============================================================================
// DeepReaL Gomoku Web - Game State Manager
// Manages board state, turn system, and game logic on the JS side
// ============================================================================

export const BOARD_SIZE = 15;
export const TOTAL_CELLS = BOARD_SIZE * BOARD_SIZE; // 225

export const CELL_EMPTY = 0;
export const CELL_BLACK = 1;
export const CELL_WHITE = 2;

export const GAME_ONGOING = -1;
export const GAME_BLACK_WIN = 1;
export const GAME_WHITE_WIN = 0;
export const GAME_DRAW = 2;

export const SIDE_BLACK = 0;
export const SIDE_WHITE = 1;

// Movetime presets (in seconds) → approximate MCTS iterations
// ~125K iters/sec based on engine README
const MOVETIME_TO_ITERS = {
    0.5: 50000,
    1:   100000,
    5:   500000,
    30:  3000000
};

export class GameState {
    constructor() {
        this.reset();
    }

    /**
     * Reset game to initial state
     */
    reset() {
        // JS-side mirror of the board (for rendering)
        this.board = new Uint8Array(TOTAL_CELLS); // 0=empty, 1=black, 2=white
        this.moveCount = 0;
        this.result = GAME_ONGOING;
        this.lastMove = -1;
        this.moveHistory = [];
        this.currentSide = SIDE_BLACK; // Black moves first
        
        // Game settings
        this.humanSide = SIDE_BLACK; // Human plays black by default
        this.movetime = 1; // Default 1 second
        
        // State flags
        this.isThinking = false;
        this.isGameOver = false;
    }

    /**
     * Get MCTS iterations for current movetime setting
     */
    getIterations() {
        return MOVETIME_TO_ITERS[this.movetime] || 100000;
    }

    /**
     * Convert (row, col) to board index
     */
    static toIndex(row, col) {
        return row * BOARD_SIZE + col;
    }

    /**
     * Convert board index to (row, col)
     */
    static toRowCol(index) {
        return {
            row: Math.floor(index / BOARD_SIZE),
            col: index % BOARD_SIZE
        };
    }

    /**
     * Check if a move is legal
     */
    isLegalMove(index) {
        if (index < 0 || index >= TOTAL_CELLS) return false;
        if (this.board[index] !== CELL_EMPTY) return false;
        if (this.result !== GAME_ONGOING) return false;
        return true;
    }

    /**
     * Record a move on the JS-side board
     */
    recordMove(index) {
        const piece = this.currentSide === SIDE_BLACK ? CELL_BLACK : CELL_WHITE;
        this.board[index] = piece;
        this.lastMove = index;
        this.moveHistory.push(index);
        this.moveCount++;
        this.currentSide = 1 - this.currentSide;
    }

    /**
     * Update game result from engine response
     */
    updateResult(engineResponse) {
        if (typeof engineResponse === 'string') {
            if (engineResponse.includes('win black')) {
                this.result = GAME_BLACK_WIN;
                this.isGameOver = true;
            } else if (engineResponse.includes('win white')) {
                this.result = GAME_WHITE_WIN;
                this.isGameOver = true;
            } else if (engineResponse.includes('draw')) {
                this.result = GAME_DRAW;
                this.isGameOver = true;
            }
        } else if (engineResponse && engineResponse.result) {
            return this.updateResult(engineResponse.result);
        }
    }

    /**
     * Check if it's the human's turn
     */
    isHumanTurn() {
        return this.currentSide === this.humanSide && !this.isGameOver && !this.isThinking;
    }

    /**
     * Check if it's the AI's turn
     */
    isAITurn() {
        return this.currentSide !== this.humanSide && !this.isGameOver && !this.isThinking;
    }

    /**
     * Get result text
     */
    getResultText() {
        switch (this.result) {
            case GAME_BLACK_WIN:
                return this.humanSide === SIDE_BLACK ? 'You Win!' : 'AI Wins!';
            case GAME_WHITE_WIN:
                return this.humanSide === SIDE_WHITE ? 'You Win!' : 'AI Wins!';
            case GAME_DRAW:
                return 'Draw!';
            default:
                return '';
        }
    }

    /**
     * Get status text for UI
     */
    getStatusText() {
        if (this.isGameOver) return this.getResultText();
        if (this.isThinking) return 'AI is thinking...';
        if (this.isHumanTurn()) return 'Your turn';
        return 'AI\'s turn';
    }

    /**
     * Get the winning line (5 in a row) if game is over
     * Returns array of indices or null
     */
    getWinningLine() {
        if (this.result !== GAME_BLACK_WIN && this.result !== GAME_WHITE_WIN) return null;
        
        const winner = this.result === GAME_BLACK_WIN ? CELL_BLACK : CELL_WHITE;
        const directions = [
            [0, 1],  // horizontal
            [1, 0],  // vertical
            [1, 1],  // diagonal \
            [1, -1]  // diagonal /
        ];
        
        for (let r = 0; r < BOARD_SIZE; r++) {
            for (let c = 0; c < BOARD_SIZE; c++) {
                if (this.board[GameState.toIndex(r, c)] !== winner) continue;
                
                for (const [dr, dc] of directions) {
                    const line = [];
                    let valid = true;
                    
                    for (let i = 0; i < 5; i++) {
                        const nr = r + dr * i;
                        const nc = c + dc * i;
                        if (nr < 0 || nr >= BOARD_SIZE || nc < 0 || nc >= BOARD_SIZE) {
                            valid = false;
                            break;
                        }
                        if (this.board[GameState.toIndex(nr, nc)] !== winner) {
                            valid = false;
                            break;
                        }
                        line.push(GameState.toIndex(nr, nc));
                    }
                    
                    if (valid && line.length === 5) return line;
                }
            }
        }
        
        return null;
    }
}

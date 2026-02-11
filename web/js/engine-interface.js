// ============================================================================
// DeepReaL Gomoku Web - Engine Interface
// Clean abstraction layer between the UI and the Web Worker
// ============================================================================

export class EngineInterface {
    constructor() {
        this.worker = null;
        this.ready = false;
        this.callbacks = new Map();
        this.pendingResolve = null;
        this.onReady = null;
        this.onError = null;
    }

    /**
     * Initialize the engine worker
     * @returns {Promise<string>} Resolves when engine is ready
     */
    init() {
        return new Promise((resolve, reject) => {
            try {
                this.worker = new Worker('js/engine-worker.js');
                
                this.worker.onmessage = (e) => {
                    this._handleMessage(e.data);
                };
                
                this.worker.onerror = (e) => {
                    const errorMsg = 'Worker error: ' + e.message;
                    if (this.onError) this.onError(errorMsg);
                    if (this.pendingResolve) {
                        this.pendingResolve.reject(new Error(errorMsg));
                        this.pendingResolve = null;
                    }
                };
                
                this.pendingResolve = { resolve, reject };
                
                // Compute the absolute URL for the WASM JS glue
                // This works on both root-hosted and subdirectory-hosted sites
                const wasmUrl = new URL('wasm/gomoku_engine.js', window.location.href).href;
                this.worker.postMessage({ type: 'init', data: { wasmUrl } });
            } catch (err) {
                reject(err);
            }
        });
    }

    /**
     * Handle incoming messages from the worker
     */
    _handleMessage(msg) {
        const { type, data, index } = msg;
        
        switch (type) {
            case 'ready':
                this.ready = true;
                if (this.pendingResolve) {
                    this.pendingResolve.resolve(data);
                    this.pendingResolve = null;
                }
                if (this.onReady) this.onReady();
                break;
                
            case 'reset':
                if (this.pendingResolve) {
                    this.pendingResolve.resolve(data);
                    this.pendingResolve = null;
                }
                break;
                
            case 'update':
                if (this.pendingResolve) {
                    this.pendingResolve.resolve({ result: data, index });
                    this.pendingResolve = null;
                }
                break;
                
            case 'bestmove':
                if (this.pendingResolve) {
                    // Parse "bestmove 112" -> 112
                    const parts = data.split(' ');
                    const moveIndex = parseInt(parts[1], 10);
                    this.pendingResolve.resolve(moveIndex);
                    this.pendingResolve = null;
                }
                break;
                
            case 'state':
                if (this.pendingResolve) {
                    this.pendingResolve.resolve(this._parseState(data));
                    this.pendingResolve = null;
                }
                break;
                
            case 'error':
                if (this.onError) this.onError(data);
                if (this.pendingResolve) {
                    this.pendingResolve.reject(new Error(data));
                    this.pendingResolve = null;
                }
                break;
                
            default:
                if (this.pendingResolve) {
                    this.pendingResolve.resolve(data);
                    this.pendingResolve = null;
                }
        }
    }

    /**
     * Parse engine state string
     */
    _parseState(data) {
        const state = {};
        const parts = data.replace('state ', '').split(' ');
        for (const part of parts) {
            const [key, value] = part.split('=');
            if (key === 'board') {
                state.board = value.split('').map(Number);
            } else {
                state[key] = parseInt(value, 10);
            }
        }
        return state;
    }

    /**
     * Play a move at the given board index
     * @param {number} index - Board position (0-224)
     * @returns {Promise<object>} Result with status
     */
    update(index) {
        return new Promise((resolve, reject) => {
            if (!this.ready) return reject(new Error('Engine not ready'));
            this.pendingResolve = { resolve, reject };
            this.worker.postMessage({ type: 'update', data: { index } });
        });
    }

    /**
     * Ask engine to find the best move
     * @param {number} iters - Number of MCTS iterations
     * @returns {Promise<number>} Best move index
     */
    go(iters = 10000) {
        return new Promise((resolve, reject) => {
            if (!this.ready) return reject(new Error('Engine not ready'));
            this.pendingResolve = { resolve, reject };
            this.worker.postMessage({ type: 'go', data: { iters } });
        });
    }

    /**
     * Reset the board to initial state
     * @returns {Promise<string>}
     */
    reset() {
        return new Promise((resolve, reject) => {
            if (!this.ready) return reject(new Error('Engine not ready'));
            this.pendingResolve = { resolve, reject };
            this.worker.postMessage({ type: 'reset' });
        });
    }

    /**
     * Get current board state
     * @returns {Promise<object>}
     */
    getState() {
        return new Promise((resolve, reject) => {
            if (!this.ready) return reject(new Error('Engine not ready'));
            this.pendingResolve = { resolve, reject };
            this.worker.postMessage({ type: 'state' });
        });
    }

    /**
     * Terminate the worker
     */
    destroy() {
        if (this.worker) {
            this.worker.terminate();
            this.worker = null;
            this.ready = false;
        }
    }
}

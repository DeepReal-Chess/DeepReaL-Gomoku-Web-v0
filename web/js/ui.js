// ============================================================================
// DeepReaL Gomoku Web - Board Renderer (Canvas)
// Neo-Futuristic Soft UI with dark glass-inspired design
// ============================================================================

import { BOARD_SIZE, CELL_BLACK, CELL_WHITE, GameState } from './game.js';

// ============================================================================
// Color Palette & Theme
// ============================================================================
const THEME = {
    // Board (lighter base so black stones stand out)
    boardBg:            '#2a3050',
    boardBgGradient1:   '#263055',
    boardBgGradient2:   '#1e2a50',
    gridLine:           'rgba(140, 170, 220, 0.35)',
    gridLineBright:     'rgba(140, 170, 220, 0.5)',
    starPoint:          'rgba(150, 180, 230, 0.6)',
    
    // Stones
    blackStone:         '#1c1c1c',
    blackStoneHighlight:'#3a3a3a',
    blackStoneRim:      'rgba(80, 120, 200, 0.4)',
    whiteStone:         '#e8e8e8',
    whiteStoneHighlight:'#ffffff',
    whiteStoneRim:      'rgba(100, 150, 255, 0.5)',
    
    // Effects
    lastMoveGlow:       'rgba(0, 200, 255, 0.7)',
    lastMoveRing:       'rgba(0, 200, 255, 0.9)',
    hoverValid:         'rgba(0, 200, 255, 0.3)',
    hoverInvalid:       'rgba(255, 50, 50, 0.2)',
    winLineGlow:        'rgba(255, 200, 50, 0.8)',
    winLineShadow:       'rgba(255, 200, 50, 0.3)',
    
    // Glass panel
    glassBg:            'rgba(30, 42, 72, 0.6)',
    glassBorder:        'rgba(120, 160, 220, 0.22)',
    glassHighlight:     'rgba(120, 160, 220, 0.06)',
};

// Star point positions (standard Gomoku 15x15)
const STAR_POINTS = [
    [3, 3], [3, 7], [3, 11],
    [7, 3], [7, 7], [7, 11],
    [11, 3], [11, 7], [11, 11]
];

export class BoardRenderer {
    constructor(canvas) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        
        // Layout constants (will be recalculated on resize)
        this.padding = 0;
        this.cellSize = 0;
        this.stoneRadius = 0;
        this.boardOffset = { x: 0, y: 0 };
        
        // Interaction state
        this.hoverCell = null; // { row, col } or null
        
        // Animation state
        this.lastPlacedStone = null; // { index, time }
        this.winLine = null;
        this.animFrame = 0;
        
        // Setup
        this._setupCanvas();
        this._bindEvents();
        
        // Click callback
        this.onCellClick = null;
    }

    /**
     * Calculate layout based on canvas size
     */
    _setupCanvas() {
        const dpr = window.devicePixelRatio || 1;
        const rect = this.canvas.getBoundingClientRect();
        const size = Math.min(rect.width, rect.height);
        
        this.canvas.width = size * dpr;
        this.canvas.height = size * dpr;
        this.canvas.style.width = size + 'px';
        this.canvas.style.height = size + 'px';
        this.ctx.scale(dpr, dpr);
        
        this.displaySize = size;
        this.padding = size * 0.06;
        this.cellSize = (size - this.padding * 2) / (BOARD_SIZE - 1);
        this.stoneRadius = this.cellSize * 0.43;
        this.boardOffset = { x: this.padding, y: this.padding };
    }

    /**
     * Bind mouse/touch events
     */
    _bindEvents() {
        this.canvas.addEventListener('mousemove', (e) => this._onMouseMove(e));
        this.canvas.addEventListener('mouseleave', () => { this.hoverCell = null; });
        this.canvas.addEventListener('click', (e) => this._onClick(e));
        
        // Touch support
        this.canvas.addEventListener('touchstart', (e) => {
            e.preventDefault();
            const touch = e.touches[0];
            const rect = this.canvas.getBoundingClientRect();
            this._onClick({
                clientX: touch.clientX,
                clientY: touch.clientY
            });
        }, { passive: false });
        
        // Resize
        window.addEventListener('resize', () => {
            this._setupCanvas();
        });
    }

    /**
     * Convert pixel coordinates to board cell
     */
    _pixelToCell(clientX, clientY) {
        const rect = this.canvas.getBoundingClientRect();
        const x = clientX - rect.left;
        const y = clientY - rect.top;
        
        const col = Math.round((x - this.boardOffset.x) / this.cellSize);
        const row = Math.round((y - this.boardOffset.y) / this.cellSize);
        
        if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) return null;
        
        // Check if close enough to intersection
        const cx = this.boardOffset.x + col * this.cellSize;
        const cy = this.boardOffset.y + row * this.cellSize;
        const dist = Math.sqrt((x - cx) ** 2 + (y - cy) ** 2);
        
        if (dist > this.cellSize * 0.5) return null;
        
        return { row, col };
    }

    _onMouseMove(e) {
        this.hoverCell = this._pixelToCell(e.clientX, e.clientY);
    }

    _onClick(e) {
        const cell = this._pixelToCell(e.clientX, e.clientY);
        if (cell && this.onCellClick) {
            this.onCellClick(cell.row, cell.col);
        }
    }

    /**
     * Resize handler
     */
    resize() {
        this._setupCanvas();
    }

    // ========================================================================
    // Rendering
    // ========================================================================

    /**
     * Main render method
     */
    render(gameState) {
        const ctx = this.ctx;
        const size = this.displaySize;
        
        // Clear
        ctx.clearRect(0, 0, size, size);
        
        // Background gradient
        this._drawBackground(ctx, size);
        
        // Glass panel effect
        this._drawGlassPanel(ctx, size);
        
        // Grid
        this._drawGrid(ctx);
        
        // Star points
        this._drawStarPoints(ctx);
        
        // Coordinate labels
        this._drawCoordinates(ctx);
        
        // Hover indicator
        if (this.hoverCell && gameState) {
            this._drawHover(ctx, gameState);
        }
        
        // Stones
        if (gameState) {
            this._drawStones(ctx, gameState);
            
            // Last move highlight
            if (gameState.lastMove >= 0) {
                this._drawLastMove(ctx, gameState.lastMove);
            }
            
            // Win line
            if (this.winLine) {
                this._drawWinLine(ctx);
            }
        }
    }

    _drawBackground(ctx, size) {
        const gradient = ctx.createLinearGradient(0, 0, size, size);
        gradient.addColorStop(0, THEME.boardBgGradient1);
        gradient.addColorStop(1, THEME.boardBgGradient2);
        ctx.fillStyle = gradient;
        ctx.fillRect(0, 0, size, size);
    }

    _drawGlassPanel(ctx, size) {
        const m = this.padding * 0.4;
        const r = 12;
        
        ctx.save();
        ctx.beginPath();
        ctx.roundRect(m, m, size - m * 2, size - m * 2, r);
        ctx.fillStyle = THEME.glassBg;
        ctx.fill();
        ctx.strokeStyle = THEME.glassBorder;
        ctx.lineWidth = 1;
        ctx.stroke();
        
        // Inner highlight (top edge)
        const hlGrad = ctx.createLinearGradient(m, m, m, m + 40);
        hlGrad.addColorStop(0, THEME.glassHighlight);
        hlGrad.addColorStop(1, 'transparent');
        ctx.fillStyle = hlGrad;
        ctx.fill();
        ctx.restore();
    }

    _drawGrid(ctx) {
        const ox = this.boardOffset.x;
        const oy = this.boardOffset.y;
        const cs = this.cellSize;
        
        ctx.strokeStyle = THEME.gridLine;
        ctx.lineWidth = 1;
        
        for (let i = 0; i < BOARD_SIZE; i++) {
            // Horizontal lines
            ctx.beginPath();
            ctx.moveTo(ox, oy + i * cs);
            ctx.lineTo(ox + (BOARD_SIZE - 1) * cs, oy + i * cs);
            ctx.stroke();
            
            // Vertical lines
            ctx.beginPath();
            ctx.moveTo(ox + i * cs, oy);
            ctx.lineTo(ox + i * cs, oy + (BOARD_SIZE - 1) * cs);
            ctx.stroke();
        }
    }

    _drawStarPoints(ctx) {
        const ox = this.boardOffset.x;
        const oy = this.boardOffset.y;
        const cs = this.cellSize;
        const radius = cs * 0.08;
        
        ctx.fillStyle = THEME.starPoint;
        
        for (const [r, c] of STAR_POINTS) {
            ctx.beginPath();
            ctx.arc(ox + c * cs, oy + r * cs, radius, 0, Math.PI * 2);
            ctx.fill();
        }
    }

    _drawCoordinates(ctx) {
        const ox = this.boardOffset.x;
        const oy = this.boardOffset.y;
        const cs = this.cellSize;
        const fontSize = Math.max(9, cs * 0.28);
        
        ctx.fillStyle = 'rgba(100, 140, 200, 0.4)';
        ctx.font = `${fontSize}px 'Inter', 'SF Pro Display', system-ui, sans-serif`;
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        
        for (let i = 0; i < BOARD_SIZE; i++) {
            // Top labels (column: A-O)
            const letter = String.fromCharCode(65 + i);
            ctx.fillText(letter, ox + i * cs, oy - this.padding * 0.55);
            
            // Left labels (row: 1-15)
            ctx.fillText((i + 1).toString(), ox - this.padding * 0.6, oy + i * cs);
        }
    }

    _drawHover(ctx, gameState) {
        const { row, col } = this.hoverCell;
        const index = GameState.toIndex(row, col);
        const ox = this.boardOffset.x + col * this.cellSize;
        const oy = this.boardOffset.y + row * this.cellSize;
        
        const isLegal = gameState.isLegalMove(index) && gameState.isHumanTurn();
        
        ctx.save();
        ctx.beginPath();
        ctx.arc(ox, oy, this.stoneRadius, 0, Math.PI * 2);
        ctx.fillStyle = isLegal ? THEME.hoverValid : THEME.hoverInvalid;
        ctx.fill();
        
        if (isLegal) {
            ctx.strokeStyle = 'rgba(0, 200, 255, 0.5)';
            ctx.lineWidth = 1.5;
            ctx.stroke();
        }
        ctx.restore();
    }

    _drawStones(ctx, gameState) {
        for (let i = 0; i < 225; i++) {
            if (gameState.board[i] === CELL_BLACK) {
                this._drawStone(ctx, i, 'black');
            } else if (gameState.board[i] === CELL_WHITE) {
                this._drawStone(ctx, i, 'white');
            }
        }
    }

    _drawStone(ctx, index, color) {
        const { row, col } = GameState.toRowCol(index);
        const cx = this.boardOffset.x + col * this.cellSize;
        const cy = this.boardOffset.y + row * this.cellSize;
        const r = this.stoneRadius;
        
        ctx.save();
        
        if (color === 'black') {
            // Black stone with subtle gradient
            const grad = ctx.createRadialGradient(cx - r * 0.3, cy - r * 0.3, r * 0.1, cx, cy, r);
            grad.addColorStop(0, THEME.blackStoneHighlight);
            grad.addColorStop(1, THEME.blackStone);
            ctx.fillStyle = grad;
            
            // Glow rim
            ctx.shadowColor = THEME.blackStoneRim;
            ctx.shadowBlur = 6;
        } else {
            // White stone with gradient
            const grad = ctx.createRadialGradient(cx - r * 0.3, cy - r * 0.3, r * 0.1, cx, cy, r);
            grad.addColorStop(0, THEME.whiteStoneHighlight);
            grad.addColorStop(1, THEME.whiteStone);
            ctx.fillStyle = grad;
            
            // Glow rim
            ctx.shadowColor = THEME.whiteStoneRim;
            ctx.shadowBlur = 8;
        }
        
        ctx.beginPath();
        ctx.arc(cx, cy, r, 0, Math.PI * 2);
        ctx.fill();
        
        // Contrasting rim: white rim on black stones, dark rim on white stones
        ctx.shadowBlur = 0;
        if (color === 'black') {
            ctx.strokeStyle = 'rgba(210, 220, 240, 0.55)';
            ctx.lineWidth = 1.5;
        } else {
            ctx.strokeStyle = 'rgba(20, 20, 40, 0.5)';
            ctx.lineWidth = 1.5;
        }
        ctx.stroke();
        
        ctx.restore();
    }

    _drawLastMove(ctx, index) {
        const { row, col } = GameState.toRowCol(index);
        const cx = this.boardOffset.x + col * this.cellSize;
        const cy = this.boardOffset.y + row * this.cellSize;
        const r = this.stoneRadius;
        
        ctx.save();
        
        // Outer glow ring
        ctx.beginPath();
        ctx.arc(cx, cy, r + 3, 0, Math.PI * 2);
        ctx.strokeStyle = THEME.lastMoveGlow;
        ctx.lineWidth = 2;
        ctx.shadowColor = THEME.lastMoveGlow;
        ctx.shadowBlur = 10;
        ctx.stroke();
        
        // Inner indicator dot
        ctx.beginPath();
        ctx.arc(cx, cy, r * 0.18, 0, Math.PI * 2);
        ctx.fillStyle = THEME.lastMoveRing;
        ctx.shadowBlur = 4;
        ctx.fill();
        
        ctx.restore();
    }

    _drawWinLine(ctx) {
        if (!this.winLine || this.winLine.length < 2) return;
        
        const first = GameState.toRowCol(this.winLine[0]);
        const last = GameState.toRowCol(this.winLine[this.winLine.length - 1]);
        
        const x1 = this.boardOffset.x + first.col * this.cellSize;
        const y1 = this.boardOffset.y + first.row * this.cellSize;
        const x2 = this.boardOffset.x + last.col * this.cellSize;
        const y2 = this.boardOffset.y + last.row * this.cellSize;
        
        ctx.save();
        
        // Glow shadow
        ctx.beginPath();
        ctx.moveTo(x1, y1);
        ctx.lineTo(x2, y2);
        ctx.strokeStyle = THEME.winLineShadow;
        ctx.lineWidth = this.stoneRadius * 1.8;
        ctx.lineCap = 'round';
        ctx.shadowColor = THEME.winLineGlow;
        ctx.shadowBlur = 20;
        ctx.stroke();
        
        // Main line
        ctx.beginPath();
        ctx.moveTo(x1, y1);
        ctx.lineTo(x2, y2);
        ctx.strokeStyle = THEME.winLineGlow;
        ctx.lineWidth = 3;
        ctx.lineCap = 'round';
        ctx.shadowBlur = 0;
        ctx.stroke();
        
        // Highlight dots on winning stones
        for (const idx of this.winLine) {
            const { row, col } = GameState.toRowCol(idx);
            const cx = this.boardOffset.x + col * this.cellSize;
            const cy = this.boardOffset.y + row * this.cellSize;
            
            ctx.beginPath();
            ctx.arc(cx, cy, this.stoneRadius * 0.25, 0, Math.PI * 2);
            ctx.fillStyle = THEME.winLineGlow;
            ctx.fill();
        }
        
        ctx.restore();
    }

    /**
     * Set the winning line for highlighting
     */
    setWinLine(line) {
        this.winLine = line;
    }

    /**
     * Clear winning line
     */
    clearWinLine() {
        this.winLine = null;
    }
}

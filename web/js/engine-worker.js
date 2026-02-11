// ============================================================================
// DeepReaL Gomoku Web - Engine Web Worker
// Loads WASM module and communicates with main thread via postMessage
// ============================================================================

let Module = null;
let engineReady = false;

async function initEngine(wasmUrl) {
    try {
        // Import the WASM JS glue from the URL provided by the main thread
        self.importScripts(wasmUrl);
        
        // Determine the directory containing the WASM files
        const wasmDir = wasmUrl.substring(0, wasmUrl.lastIndexOf('/') + 1);
        
        // Initialize with locateFile so Emscripten finds the .wasm binary
        Module = await GomokuEngine({
            locateFile: (path) => wasmDir + path
        });
        const result = Module.engineInit();
        engineReady = true;
        self.postMessage({ type: 'ready', data: result });
    } catch (err) {
        self.postMessage({ type: 'error', data: 'Failed to load engine: ' + err.message });
    }
}

// Handle messages from main thread
self.onmessage = function(e) {
    const { type, data } = e.data;
    
    if (!engineReady && type !== 'init') {
        self.postMessage({ type: 'error', data: 'Engine not ready' });
        return;
    }
    
    try {
        switch (type) {
            case 'init':
                initEngine(data.wasmUrl);
                break;
                
            case 'reset': {
                const result = Module.engineReset();
                self.postMessage({ type: 'reset', data: result });
                break;
            }
            
            case 'update': {
                const result = Module.engineUpdate(data.index);
                self.postMessage({ type: 'update', data: result, index: data.index });
                break;
            }
            
            case 'go': {
                const iters = data.iters || 10000;
                const result = Module.engineGo(iters);
                self.postMessage({ type: 'bestmove', data: result });
                break;
            }
            
            case 'state': {
                const result = Module.engineGetState();
                self.postMessage({ type: 'state', data: result });
                break;
            }
            
            case 'command': {
                const result = Module.engineCommand(data.cmd);
                self.postMessage({ type: 'command', data: result });
                break;
            }
            
            default:
                self.postMessage({ type: 'error', data: 'Unknown command: ' + type });
        }
    } catch (err) {
        self.postMessage({ type: 'error', data: err.message });
    }
};

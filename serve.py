#!/usr/bin/env python3
"""Simple HTTP server with no-cache headers for development."""
import http.server
import functools
import os

class NoCacheHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate, max-age=0')
        self.send_header('Pragma', 'no-cache')
        self.send_header('Expires', '0')
        super().end_headers()

if __name__ == '__main__':
    os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'web'))
    server = http.server.HTTPServer(('', 8080), NoCacheHTTPRequestHandler)
    print(f'Serving at http://localhost:8080 (no-cache)')
    server.serve_forever()

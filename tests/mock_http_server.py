import http.server
import socketserver
import time
import json

PORT = 8080

class MockHTTPRequestHandler(http.server.BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        print(f"[MockServer] {self.address_string()} - {format % args}")

    def do_GET(self):
        path = self.path.split('?')[0]

        if path == '/404':
            self.send_response(404)
            self.send_header('Content-Type', 'text/html')
            self.end_headers()
            self.wfile.write(b'404 Not Found')

        elif path == '/500':
            self.send_response(500)
            self.send_header('Content-Type', 'text/html')
            self.end_headers()
            self.wfile.write(b'500 Internal Server Error')

        elif path == '/timeout':
            print(f"[MockServer] Handling /timeout - will hang for 30s...")
            time.sleep(30)
            self.send_response(200)
            self.end_headers()

        elif path == '/empty':
            self.send_response(200)
            self.send_header('Content-Type', 'application/zip')
            self.send_header('Content-Length', '0')
            self.end_headers()

        elif path.startswith('/file'):
            filename = path.split('/')[-1]
            if '.zip' not in filename:
                filename = filename + '.zip'
            content = b'X' * (1024 * 1024)
            self.send_response(200)
            self.send_header('Content-Type', 'application/zip')
            self.send_header('Content-Length', str(len(content)))
            self.send_header('Content-Disposition', f'attachment; filename="{filename}"')
            self.end_headers()
            self.wfile.write(content)

        elif path == '/redirect':
            self.send_response(302)
            self.send_header('Location', '/file1.zip')
            self.end_headers()

        elif path == '/special':
            content = b'test data'
            filename = "test_file(1).zip"
            self.send_response(200)
            self.send_header('Content-Type', 'application/zip')
            self.send_header('Content-Length', str(len(content)))
            self.send_header('Content-Disposition', f'attachment; filename="{filename}"')
            self.end_headers()
            self.wfile.write(content)

        elif path == '/health':
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps({"status": "ok"}).encode())

        else:
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b'Not Found')

    def do_HEAD(self):
        path = self.path.split('?')[0]
        if path.startswith('/file'):
            self.send_response(200)
            self.send_header('Content-Type', 'application/zip')
            self.send_header('Content-Length', str(1024 * 1024))
            self.send_header('Accept-Ranges', 'bytes')
            self.end_headers()
        else:
            self.send_response(404)
            self.end_headers()


def run_server():
    with socketserver.TCPServer(("", PORT), MockHTTPRequestHandler) as httpd:
        print(f"[MockServer] HTTP Server started on port {PORT}")
        print(f"  - http://127.0.0.1:{PORT}/404")
        print(f"  - http://127.0.0.1:{PORT}/500")
        print(f"  - http://127.0.0.1:{PORT}/timeout")
        print(f"  - http://127.0.0.1:{PORT}/empty")
        print(f"  - http://127.0.0.1:{PORT}/file1.zip")
        print(f"  - http://127.0.0.1:{PORT}/redirect")
        print(f"  - http://127.0.0.1:{PORT}/health")
        print(f"\nPress Ctrl+C to stop...")
        httpd.serve_forever()


if __name__ == "__main__":
    run_server()

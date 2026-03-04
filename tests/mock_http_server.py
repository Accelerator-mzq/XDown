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

        elif path == '/403':
            # GAP-5: 403 禁止访问测试
            self.send_response(403)
            self.send_header('Content-Type', 'text/html')
            self.end_headers()
            self.wfile.write(b'403 Forbidden')

        elif path == '/resumable':
            # GAP-4: 断点续传测试 (支持 Range)
            total_size = 1024 * 1024
            content = b'R' * total_size
            range_header = self.headers.get('Range')
            if range_header:
                start = int(range_header.split('=')[1].rstrip('-'))
                partial = content[start:]
                self.send_response(206)
                self.send_header('Content-Range', f'bytes {start}-{total_size-1}/{total_size}')
                self.send_header('Content-Length', str(len(partial)))
                self.send_header('Accept-Ranges', 'bytes')
                self.end_headers()
                self.wfile.write(partial)
            else:
                self.send_response(200)
                self.send_header('Content-Length', str(total_size))
                self.send_header('Accept-Ranges', 'bytes')
                self.end_headers()
                self.wfile.write(content)

        elif path == '/no-resume':
            # GAP-4: 服务器不支持续传 (忽略 Range，始终返回 200)
            content = b'N' * (512 * 1024)
            self.send_response(200)
            self.send_header('Content-Length', str(len(content)))
            self.end_headers()
            self.wfile.write(content)

        elif path == '/redirect-chain':
            # GAP-3: 重定向链测试 (3跳)
            self.send_response(302)
            self.send_header('Location', '/redirect-chain-2')
            self.end_headers()

        elif path == '/redirect-chain-2':
            self.send_response(302)
            self.send_header('Location', '/redirect-chain-3')
            self.end_headers()

        elif path == '/redirect-chain-3':
            self.send_response(302)
            self.send_header('Location', '/file1.zip')
            self.end_headers()

        elif path == '/redirect-loop':
            # GAP-3: 无限重定向循环测试
            self.send_response(302)
            self.send_header('Location', '/redirect-loop')
            self.end_headers()

        elif path == '/named/report.pdf':
            # GAP-2: 文件名自动解析测试 - 带文件名
            content = b'N' * 2048
            self.send_response(200)
            self.send_header('Content-Type', 'application/pdf')
            self.send_header('Content-Length', str(len(content)))
            self.send_header('Content-Disposition', 'attachment; filename="report.pdf"')
            self.end_headers()
            self.wfile.write(content)

        elif path == '/noname' or path == '/noname/':
            # GAP-2: 文件名自动解析测试 - 无文件名
            content = b'U' * 2048
            self.send_response(200)
            self.send_header('Content-Type', 'application/octet-stream')
            self.send_header('Content-Length', str(len(content)))
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
        print(f"  - http://127.0.0.1:{PORT}/redirect-chain")
        print(f"  - http://127.0.0.1:{PORT}/redirect-loop")
        print(f"  - http://127.0.0.1:{PORT}/403")
        print(f"  - http://127.0.0.1:{PORT}/resumable")
        print(f"  - http://127.0.0.1:{PORT}/no-resume")
        print(f"  - http://127.0.0.1:{PORT}/named/report.pdf")
        print(f"  - http://127.0.0.1:{PORT}/noname")
        print(f"  - http://127.0.0.1:{PORT}/health")
        print(f"\nPress Ctrl+C to stop...")
        httpd.serve_forever()


if __name__ == "__main__":
    run_server()

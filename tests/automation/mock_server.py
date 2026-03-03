#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
XDown 自动化测试 - Mock HTTP 服务器
用于模拟各种 HTTP 响应场景
"""

import http.server
import socketserver
import threading
import time
import os
import json

# 配置端口
DEFAULT_PORT = 8080

class MockHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    """自定义 Mock HTTP 请求处理器"""

    def do_GET(self):
        """处理 GET 请求"""
        # 路由配置
        routes = {
            '/404': self.handle_404,
            '/500': self.handle_500,
            '/timeout': self.handle_timeout,
            '/empty': self.handle_empty,
            '/slow': self.handle_slow,
            '/file1.zip': self.handle_file1,
            '/file2.zip': self.handle_file2,
            '/file3.zip': self.handle_file3,
            '/file4.zip': self.handle_file4,
            '/file5.zip': self.handle_file5,
        }

        handler = routes.get(self.path)
        if handler:
            handler()
        else:
            # 默认返回 200 和测试文件
            self.handle_file1()

    def handle_404(self):
        """ST-5.1: 404 错误"""
        self.send_response(404)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', '21')
        self.end_headers()
        self.wfile.write(b'{"error": "Not Found"}')

    def handle_500(self):
        """ST-5.5: 500 服务器错误"""
        self.send_response(500)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', '28')
        self.end_headers()
        self.wfile.write(b'{"error": "Internal Server Error"}')

    def handle_timeout(self):
        """ST-5.2: 网络超时 - 延迟响应"""
        time.sleep(35)  # 延迟 35 秒
        self.send_response(200)
        self.end_headers()

    def handle_slow(self):
        """慢速响应"""
        time.sleep(5)
        content = b'x' * 1024
        self.send_response(200)
        self.send_header('Content-Length', str(len(content)))
        self.end_headers()
        self.wfile.write(content)

    def handle_empty(self):
        """ST-8.1: 0 字节文件"""
        self.send_response(200)
        self.send_header('Content-Length', '0')
        self.end_headers()

    def handle_file1(self):
        """测试文件 1 - 1KB"""
        content = b'x' * 1024
        self.send_response(200)
        self.send_header('Content-Length', str(len(content)))
        self.send_header('Content-Disposition', 'attachment; filename="test1.zip"')
        self.send_header('Content-Type', 'application/zip')
        self.end_headers()
        self.wfile.write(content)

    def handle_file2(self):
        """测试文件 2 - 1KB"""
        content = b'y' * 1024
        self.send_response(200)
        self.send_header('Content-Length', str(len(content)))
        self.send_header('Content-Disposition', 'attachment; filename="test2.zip"')
        self.send_header('Content-Type', 'application/zip')
        self.end_headers()
        self.wfile.write(content)

    def handle_file3(self):
        """测试文件 3 - 1KB"""
        content = b'z' * 1024
        self.send_response(200)
        self.send_header('Content-Length', str(len(content)))
        self.send_header('Content-Disposition', 'attachment; filename="test3.zip"')
        self.send_header('Content-Type', 'application/zip')
        self.end_headers()
        self.wfile.write(content)

    def handle_file4(self):
        """测试文件 4 - 1KB"""
        content = b'a' * 1024
        self.send_response(200)
        self.send_header('Content-Length', str(len(content)))
        self.send_header('Content-Disposition', 'attachment; filename="test4.zip"')
        self.send_header('Content-Type', 'application/zip')
        self.end_headers()
        self.wfile.write(content)

    def handle_file5(self):
        """测试文件 5 - 1KB"""
        content = b'b' * 1024
        self.send_response(200)
        self.send_header('Content-Length', str(len(content)))
        self.send_header('Content-Disposition', 'attachment; filename="test5.zip"')
        self.send_header('Content-Type', 'application/zip')
        self.end_headers()
        self.wfile.write(content)

    def log_message(self, format, *args):
        """抑制 HTTP 服务器日志输出"""
        pass


class ThreadedHTTPServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    """多线程 HTTP 服务器"""
    allow_reuse_address = True


def start_server(port=DEFAULT_PORT):
    """启动 Mock HTTP 服务器"""
    server = ThreadedHTTPServer(('127.0.0.1', port), MockHTTPRequestHandler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    return server


def stop_server(server):
    """停止 Mock HTTP 服务器"""
    if server:
        server.shutdown()


if __name__ == '__main__':
    print(f"Starting Mock HTTP Server on port {DEFAULT_PORT}...")
    server = start_server(DEFAULT_PORT)
    print(f"Mock server running at http://127.0.0.1:{DEFAULT_PORT}")
    print("Press Ctrl+C to stop")

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nStopping server...")
        stop_server(server)
        print("Server stopped")

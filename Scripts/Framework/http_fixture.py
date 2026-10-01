"""Loopback-only fixture for MH.Backend.Http.Loopback. No external services."""
import argparse
import socket
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *_):
        pass

    def reply(self, status, body):
        self.send_response(status)
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        try:
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
            pass  # Timeout and cancellation deliberately disconnect.

    def do_GET(self):
        if self.path == "/disconnect":
            self.connection.shutdown(socket.SHUT_RDWR)
            self.connection.close()
            self.close_connection = True
        elif self.path == "/slow":
            time.sleep(2)
            self.reply(200, b"slow")
        elif self.path == "/ok":
            self.reply(200, b"ok")
        else:
            self.reply(404, b"missing")

    def do_POST(self):
        size = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(size)
        if (self.path == "/echo" and self.headers.get("X-MH-Fixture") == "present"
                and self.headers.get("Content-Type") == "application/octet-stream"):
            self.reply(201, body)
        else:
            self.reply(400, b"bad fixture request")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port-file", required=True)
    args = parser.parse_args()
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    Path(args.port_file).write_text(str(server.server_port), encoding="ascii")
    try:
        server.serve_forever()
    finally:
        server.server_close()

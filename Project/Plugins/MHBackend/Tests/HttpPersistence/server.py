"""Loopback-only, in-memory fixture for the MH v1 opaque record protocol. Not production."""
import argparse
import base64
import binascii
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


def decode_key(encoded):
    if not encoded or any(c not in "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_" for c in encoded):
        raise ValueError("Invalid key encoding")
    data = base64.urlsafe_b64decode(encoded + "=" * (-len(encoded) % 4))
    if base64.urlsafe_b64encode(data).decode().rstrip("=") != encoded:
        raise ValueError("Noncanonical key")
    text = data.decode("utf-8")
    if not text or any(ord(c) < 32 or ord(c) == 127 for c in text):
        raise ValueError("Invalid key")
    return text


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *_):
        pass

    def reply(self, status, body=b""):
        self.send_response(status)
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        try:
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
            pass

    def dispatch(self):
        if self.headers.get("Authorization") != "Bearer mh-local-test-only":
            self.reply(401)
            return
        path = self.path
        # Deterministic fault injection, confined to this test service.
        if path.startswith("/unexpected/"):
            self.reply(202)
            return
        if path.startswith("/unavailable/"):
            self.reply(503)
            return
        if path.startswith("/slow/"):
            time.sleep(2)
            path = path[len("/slow"):]
        parts = path.split("/")
        try:
            if len(parts) != 5 or parts[1:3] != ["v1", "records"]:
                raise ValueError("Invalid route")
            key = (decode_key(parts[3]), decode_key(parts[4]))
            # Match Unreal's UTF-16 length bounds for valid Unicode strings.
            if len(key[0].encode("utf-16-le")) // 2 > 128 or len(key[1].encode("utf-16-le")) // 2 > 512:
                raise ValueError("Key too long")
            size = int(self.headers.get("Content-Length", "0"))
            if size < 0 or size > 1024 * 1024 or self.headers.get("Transfer-Encoding"):
                self.reply(413)
                return
            if self.command == "PUT" and self.headers.get("Content-Type") != "application/octet-stream":
                self.reply(415)
                return
            body = self.rfile.read(size) if self.command == "PUT" else b""
            if self.command == "PUT" and len(body) != size:
                raise ValueError("Incomplete body")
        except (ValueError, UnicodeError, binascii.Error):
            self.reply(400)
            return
        with self.server.records_lock:
            if self.command == "GET":
                if key in self.server.records:
                    status, output = 200, self.server.records[key]
                else:
                    status, output = 404, b""
            elif self.command == "PUT":
                self.server.records[key] = body
                status, output = 204, b""
            else:
                self.server.records.pop(key, None)
                status, output = 204, b""
        self.reply(status, output)

    do_GET = dispatch
    do_PUT = dispatch
    do_DELETE = dispatch


def make_server():
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    server.records = {}
    server.records_lock = threading.Lock()
    server.daemon_threads = True
    return server


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port-file", required=True)
    args = parser.parse_args()
    server = make_server()
    Path(args.port_file).write_text(str(server.server_port), encoding="ascii")
    try:
        server.serve_forever()
    finally:
        server.server_close()

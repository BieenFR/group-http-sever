"""Integration checks against the REAL sws binary, never a substitute server."""
from pathlib import Path
import socket, subprocess, tempfile, time
root = Path(__file__).resolve().parents[1]
def exchange(port, request):
    with socket.create_connection(('127.0.0.1', port), timeout=2) as client:
        client.settimeout(3)
        client.sendall(request)
        chunks = []
        while True:
            data = client.recv(65536)
            if not data: break
            chunks.append(data)
    raw = b''.join(chunks)
    headers, separator, body = raw.partition(b'\r\n\r\n')
    assert separator, 'Response needs CRLF headers and blank line'
    return headers, body
with tempfile.TemporaryDirectory() as tmp:
    docroot = Path(tmp) / 'www'
    docroot.mkdir()
    html = b'<h1>HTTP group</h1>\n'
    image = b'\x89PNG\r\n\x1a\n' + bytes(range(256))
    (docroot / 'index.html').write_bytes(html)
    (docroot / 'sample.png').write_bytes(image)
    with socket.socket() as probe:
        probe.bind(('127.0.0.1', 0))
        port = probe.getsockname()[1]
    proc = subprocess.Popen([str(root/'sws'), '-d', '-i', '127.0.0.1', '-p', str(port), str(docroot)], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    try:
        ready = False
        for _ in range(60):
            if proc.poll() is not None:
                raise AssertionError('sws exited before listening: ' + proc.stderr.read().decode())
            try:
                with socket.create_connection(('127.0.0.1', port), timeout=.1): pass
                ready = True
                break
            except OSError: time.sleep(.05)
        assert ready, 'sws failed to listen within 3 seconds'
        headers, body = exchange(port, b'GET / HTTP/1.0\r\n\r\n')
        assert headers.startswith(b'HTTP/1.0 200 '), headers
        assert body == html
        assert b'content-length: ' + str(len(html)).encode() in headers.lower()
        head, body = exchange(port, b'HEAD / HTTP/1.0\r\n\r\n')
        assert head.startswith(b'HTTP/1.0 200 ') and body == b''
        assert b'content-length: ' + str(len(html)).encode() in head.lower()
        headers, body = exchange(port, b'GET /sample.png HTTP/1.0\r\n\r\n')
        assert headers.startswith(b'HTTP/1.0 200 ') and body == image
        headers, _ = exchange(port, b'GET /missing HTTP/1.0\r\n\r\n')
        assert headers.startswith(b'HTTP/1.0 404 ')
        headers, body = exchange(port, b'GET / HTTP/1.1\r\nHost: localhost\r\n\r\n')
        assert headers.startswith(b'HTTP/1.0 200 ') and body == html
        print('PASS: real GET, HEAD, binary transfer, 404 and HTTP/1.1 acceptance.')
    finally:
        proc.terminate()
        try: proc.wait(timeout=3)
        except subprocess.TimeoutExpired: proc.kill(); proc.wait()
        proc.stderr.close()

"""Integration tests against the real, combined server; no HTTP module doubles."""
from pathlib import Path
import base64
import email.utils
import os
import re
import shutil
import socket
import subprocess
import tempfile
import time
import unittest
import sys

ROOT = Path(__file__).resolve().parents[1]
PNG = base64.b64decode('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+jD1sAAAAASUVORK5CYII=')

class HTTPTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(prefix='sws-http-')
        cls.work = Path(cls.tmp.name)
        cls.web = cls.work/'www'; cls.web.mkdir()
        cls.cgi = cls.work/'cgi'; cls.cgi.mkdir()
        cls.html = b'<!doctype html><html><title>Group</title><h1>Team HTTP</h1></html>\n'
        (cls.web/'index.html').write_bytes(cls.html)
        (cls.web/'real.png').write_bytes(PNG)
        (cls.web/'style.css').write_text('body { color: navy; }\n')
        (cls.web/'fake.png').write_text('This file is plain text, despite its extension.\n')
        (cls.web/'large.bin').write_bytes(bytes(range(256))*4096)
        (cls.web/'list').mkdir()
        for name in ['b.txt', 'a.txt', 'a b.txt', '<tag>.txt', '.secret']:
            (cls.web/'list'/name).write_text(name)
        (cls.work/'secret.txt').write_text('outside-root')
        (cls.web/'escape').symlink_to(cls.work/'secret.txt')
        (cls.web/'inner').symlink_to(cls.web/'fake.png')
        cls.script('hello.sh', "printf 'Content-Type: text/plain\\r\\n\\r\\nhello CGI\\n'\n")
        cls.script('env.sh', "printf 'Content-Type: text/plain\\r\\n\\r\\n'\nprintf '%s|%s|%s|%s|%s|%s|%s' \"$REQUEST_METHOD\" \"$QUERY_STRING\" \"$PATH_INFO\" \"$SCRIPT_NAME\" \"$SERVER_PROTOCOL\" \"$REMOTE_ADDR\" \"$SERVER_PORT\"\n")
        cls.script('bad.sh', "printf 'no header at all'\n")
        cls.script('fail.sh', 'exit 3\n')
        cls.script('slow.sh', 'sleep 8\n')
        cls.script('closed.sh', 'exec 1>&-\nsleep 8\n')
        cls.script('redirect.sh', "printf 'Location: /index.html\\r\\n\\r\\n'\n")
        cls.script('loop.sh', "printf 'Location: /cgi-bin/loop.sh\\r\\n\\r\\n'\n")
        cls.script('external.sh', "printf 'Location: https://example.com/\\r\\n\\r\\n'\n")
        cls.script('status.sh', "printf 'Status: 201 Created\\r\\nContent-Type: text/plain\\r\\nSet-Cookie: one=1\\r\\nSet-Cookie: two=2\\r\\nX-Test: okay\\r\\n\\r\\ncreated'\n")
        cls.script('inject.sh', "printf 'Content-Type: text/plain\\r\\nTransfer-Encoding: chunked\\r\\n\\r\\nbad'\n")
        cls.script('huge.sh', "printf 'Content-Type: text/plain\\r\\n\\r\\n'\nhead -c 4200000 /dev/zero\n")
        cls.script('head.sh', "printf 'Content-Type: text/plain\\r\\nContent-Length: 9\\r\\n\\r\\n'\n[ \"$REQUEST_METHOD\" = HEAD ] || printf '123456789'\n")
        cls.script('binary.sh', "printf 'Content-Type: application/octet-stream\\r\\n\\r\\nA\\000B'\n")
        (cls.cgi/'escape.sh').symlink_to(cls.work/'secret.txt')
        (cls.cgi/'nonexec').write_text('hello')
        with socket.socket() as probe:
            probe.bind(('127.0.0.1', 0)); cls.port = probe.getsockname()[1]
        cls.stdout = tempfile.TemporaryFile()
        cls.stderr = tempfile.TemporaryFile()
        cls.proc = subprocess.Popen([str(ROOT/'sws'), '-d', '-i', '127.0.0.1', '-p', str(cls.port), '-c', str(cls.cgi), str(cls.web)], stdout=cls.stdout, stderr=cls.stderr)
        for _ in range(100):
            try:
                with socket.create_connection(('127.0.0.1', cls.port), timeout=.1): pass
                break
            except OSError: time.sleep(.02)
        else: raise RuntimeError('real server did not listen')
    @classmethod
    def script(cls, name, source):
        p = cls.cgi/name; p.write_text('#!/bin/sh\n'+source); p.chmod(0o755)
    @classmethod
    def tearDownClass(cls):
        cls.proc.terminate()
        try: cls.proc.wait(timeout=4)
        except subprocess.TimeoutExpired: cls.proc.kill(); cls.proc.wait()
        cls.stdout.close(); cls.stderr.close(); cls.tmp.cleanup()
    def raw(self, request, fragments=None):
        with socket.create_connection(('127.0.0.1', self.port), timeout=3) as client:
            client.settimeout(10)
            if fragments:
                for item in fragments: client.sendall(item); time.sleep(.02)
            else: client.sendall(request)
            data = bytearray()
            while True:
                try: chunk = client.recv(65536)
                except ConnectionResetError: break
                if not chunk: break
                data.extend(chunk)
        head, separator, body = bytes(data).partition(b'\r\n\r\n')
        self.assertTrue(separator, bytes(data))
        lines = head.split(b'\r\n')
        self.assertTrue(lines[0].startswith(b'HTTP/1.0 '), lines[0])
        status = int(lines[0].split()[1])
        headers = {}
        for line in lines[1:]:
            name, value = line.split(b':', 1)
            headers.setdefault(name.decode().lower(), []).append(value.strip().decode())
        return status, headers, body
    def get(self, path='/', method='GET', headers='', version='1.0'):
        return self.raw(f'{method} {path} HTTP/{version}\r\n{headers}\r\n'.encode())
    def test_get_html(self):
        code, headers, body = self.get()
        self.assertEqual((code, body), (200, self.html))
        for name in ['date','server','last-modified','content-type','content-length']:
            self.assertIn(name, headers)
        self.assertEqual(int(headers['content-length'][0]), len(body))
        self.assertEqual(headers['content-type'], ['text/html'])
    def test_head(self):
        code, headers, body = self.get(method='HEAD')
        self.assertEqual((code, body), (200, b''))
        self.assertEqual(int(headers['content-length'][0]), len(self.html))
    def test_http11(self):
        self.assertEqual(self.get(version='1.1', headers='Host: localhost\r\n')[0], 200)
    def test_fragmented_request(self):
        self.assertEqual(self.raw(b'', fragments=[b'GET / HTTP/1.', b'1\r\nHost: ', b'localhost\r\n', b'\r\n'])[0], 200)
    def test_404_and_head_error(self):
        self.assertEqual(self.get('/missing')[0], 404)
        code, headers, body = self.get('/missing', method='HEAD')
        self.assertEqual((code, body), (404, b''))
        self.assertGreater(int(headers['content-length'][0]), 0)
    def test_unsupported_method(self):
        self.assertEqual(self.get(method='POST')[0], 501)
    def test_bad_requests(self):
        for request in [b'GET / HTTP/2.0\r\n\r\n', b'GET http://example.com HTTP/1.0\r\n\r\n',
                        b'GET / HTTP/1.0\r\nBad header\r\n\r\n', b'GET / HTTP/1.0\r\n: x\r\n\r\n',
                        b'GET / HTTP/1.0\r\nX: a\x00b\r\n\r\n', b'GET / HTTP/1.0\r\n folded: x\r\n\r\n']:
            with self.subTest(request=request): self.assertEqual(self.raw(request)[0], 400)
    def test_oversized(self):
        self.assertEqual(self.raw(b'GET / HTTP/1.0\r\nX: '+b'a'*17000+b'\r\n\r\n')[0], 400)
    def test_binary_and_magic(self):
        code, headers, body = self.get('/real.png')
        self.assertEqual((code, body), (200, PNG)); self.assertEqual(headers['content-type'], ['image/png'])
        self.assertEqual(self.get('/fake.png')[1]['content-type'], ['text/plain'])
    def test_stylesheet_mime(self):
        self.assertEqual(self.get('/style.css')[1]['content-type'], ['text/css'])
    def test_cgi_disabled(self):
        with socket.socket() as probe:
            probe.bind(('127.0.0.1',0)); port=probe.getsockname()[1]
        with tempfile.TemporaryFile() as stdout, tempfile.TemporaryFile() as stderr:
            proc=subprocess.Popen([str(ROOT/'sws'),'-d','-i','127.0.0.1','-p',str(port),str(self.web)],stdout=stdout,stderr=stderr)
            try:
                for _ in range(100):
                    try: client=socket.create_connection(('127.0.0.1',port),timeout=.1); break
                    except OSError: time.sleep(.02)
                else: self.fail('disabled-CGI server not listening')
                with client:
                    client.sendall(b'GET /cgi-bin/hello.sh HTTP/1.0\r\n\r\n')
                    self.assertTrue(client.recv(4096).startswith(b'HTTP/1.0 403 '))
            finally:
                proc.terminate(); proc.wait(timeout=4)
    def test_large_binary(self):
        code, headers, body = self.get('/large.bin')
        self.assertEqual(code, 200); self.assertEqual(body, bytes(range(256))*4096)
        self.assertEqual(int(headers['content-length'][0]), len(body))
    def test_query_static(self):
        self.assertEqual(self.get('/?q=test')[2], self.html)
    def test_modified_since(self):
        modified = int((self.web/'index.html').stat().st_mtime)
        for date in [email.utils.formatdate(modified, usegmt=True),
                     time.strftime('%A, %d-%b-%y %H:%M:%S GMT', time.gmtime(modified)),
                     time.asctime(time.gmtime(modified))]:
            with self.subTest(date=date):
                code, _, body = self.get(headers='iF-MoDiFiEd-SiNcE: '+date+'\r\n')
                self.assertEqual((code, body), (304, b''))
        self.assertEqual(self.get(headers='If-Modified-Since: '+email.utils.formatdate(modified-100, usegmt=True)+'\r\n')[0], 200)
        self.assertEqual(self.get(headers='If-Modified-Since: nonsense\r\n')[0], 200)
        self.assertEqual(self.get(headers='If-Modified-Since: Thu, 99 Oct 2026 00:00:00 GMT\r\n')[0], 200)
    def test_future_modified_since_ignored(self):
        self.assertEqual(self.get(headers='If-Modified-Since: '+email.utils.formatdate(time.time()+86400, usegmt=True)+'\r\n')[0],200)
    def test_cgi_limits_and_head_length(self):
        self.assertEqual(self.get('/cgi-bin/huge.sh')[0],502)
        code,headers,body=self.get('/cgi-bin/head.sh',method='HEAD')
        self.assertEqual((code,body),(200,b'')); self.assertEqual(headers['content-length'],['9'])
    def test_duplicate_date(self):
        self.assertEqual(self.get(headers='If-Modified-Since: x\r\nIf-Modified-Since: x\r\n')[0], 400)
    def test_listing(self):
        code, _, body = self.get('/list')
        self.assertEqual(code, 200)
        self.assertNotIn(b'.secret', body); self.assertIn(b'&lt;tag&gt;.txt', body)
        self.assertIn(b'/list/a%20b.txt', body)
        self.assertLess(body.index(b'>a.txt<'), body.index(b'>b.txt<'))
        self.assertEqual(self.get('/list/a%20b.txt')[2], b'a b.txt')
        self.assertEqual(self.get('/list/%3Ctag%3E.txt')[2], b'<tag>.txt')
    def test_head_listing(self):
        _, full_headers, full_body = self.get('/list/')
        code, headers, body = self.get('/list/', method='HEAD')
        self.assertEqual((code, body), (200, b''))
        self.assertEqual(headers['content-length'], full_headers['content-length'])
        self.assertEqual(int(headers['content-length'][0]), len(full_body))
    def test_path_safety(self):
        for path in ['/../secret.txt', '/%2e%2e/secret.txt', '/escape', '/%00', '/bad%xx', '/%5c../secret.txt']:
            with self.subTest(path=path): self.assertIn(self.get(path)[0], (403,404))
        self.assertEqual(self.get('/inner')[0], 200)
    def test_user_missing(self):
        self.assertEqual(self.get('/~sws_user_that_does_not_exist_991/')[0], 404)
    def test_cgi(self):
        code, headers, body = self.get('/cgi-bin/hello.sh')
        self.assertEqual((code, body), (200,b'hello CGI\n'))
        self.assertEqual(int(headers['content-length'][0]), len(body))
    def test_cgi_environment_and_path_info(self):
        code, _, body = self.get('/cgi-bin/env.sh/more?x=1&y=2')
        self.assertEqual(code, 200)
        self.assertEqual(body.decode().split('|'), ['GET','x=1&y=2','/more','/cgi-bin/env.sh','HTTP/1.0','127.0.0.1',str(self.port)])
    def test_cgi_head(self):
        code, headers, body = self.get('/cgi-bin/hello.sh', method='HEAD')
        self.assertEqual((code, body), (200,b'')); self.assertEqual(int(headers['content-length'][0]), len(b'hello CGI\n'))
    def test_cgi_binary(self):
        self.assertEqual(self.get('/cgi-bin/binary.sh')[2], b'A\x00B')
    def test_cgi_status_and_extension_headers(self):
        code, headers, body = self.get('/cgi-bin/status.sh')
        self.assertEqual((code,body), (201,b'created'))
        self.assertEqual(headers['set-cookie'], ['one=1','two=2']); self.assertEqual(headers['x-test'], ['okay'])
    def test_cgi_redirects(self):
        self.assertEqual(self.get('/cgi-bin/redirect.sh')[2], self.html)
        code, headers, _ = self.get('/cgi-bin/external.sh')
        self.assertEqual(code,302); self.assertEqual(headers['location'], ['https://example.com/'])
        self.assertEqual(self.get('/cgi-bin/loop.sh')[0], 502)
    def test_cgi_errors(self):
        for name, expected in [('bad.sh',502),('fail.sh',502),('nonexec',403),('missing',404),('escape.sh',403),('inject.sh',502)]:
            with self.subTest(name=name): self.assertEqual(self.get('/cgi-bin/'+name)[0], expected)
    def test_cgi_timeout(self):
        start=time.monotonic()
        self.assertEqual(self.get('/cgi-bin/slow.sh')[0],504)
        self.assertLess(time.monotonic()-start,7)
        self.assertEqual(self.get()[0],200)
    def test_cgi_closed_stdout_timeout(self):
        self.assertEqual(self.get('/cgi-bin/closed.sh')[0],504)
    def test_real_debug_log(self):
        self.get('/?log-check=1')
        time.sleep(.05)
        self.stdout.seek(0); text=self.stdout.read().decode()
        self.assertRegex(text, r'127\.0\.0\.1 \d{4}-\d\d-\d\dT\d\d:\d\d:\d\dZ "GET /\?log-check=1 HTTP/1.0" 200 \d+')

if __name__ == '__main__':
    if '--static' in sys.argv:
        suite=unittest.TestSuite(HTTPTests(name) for name in unittest.defaultTestLoader.getTestCaseNames(HTTPTests)
                                if 'cgi' not in name and name != 'test_real_debug_log')
        result=unittest.TextTestRunner(verbosity=2).run(suite)
        raise SystemExit(not result.wasSuccessful())
    unittest.main(verbosity=2)

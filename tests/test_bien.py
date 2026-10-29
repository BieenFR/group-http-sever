"""Test Bien's real socket/process/config code with explicitly isolated module doubles.
These tests do NOT validate HTTP parsing, responses, CGI, or the production logger.
Linux/Ubuntu only so far; run on reference hosts separately.
"""
from pathlib import Path
import os
import signal
import socket
import subprocess
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]

class BienTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(prefix='sws-bien-')
        cls.work = Path(cls.tmp.name)
        cls.binary = cls.work / 'server-driver'
        (cls.work / 'www').mkdir()
        (cls.work / 'cgi').mkdir()
        subprocess.run(['cc', '-D_POSIX_C_SOURCE=200809L', '-DSWS_REQUEST_TIMEOUT=1',
                        '-I'+str(ROOT/'include'), '-std=c11', '-Wall', '-Wextra',
                        '-Wpedantic', '-Werror', '-o', str(cls.binary),
                        str(ROOT/'src/main.c'), str(ROOT/'src/config.c'),
                        str(ROOT/'src/server.c'), str(ROOT/'tests/fixtures/server_modules.c')], check=True)
    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()
    def setUp(self):
        self.processes = []
        self.daemons = []
    def tearDown(self):
        for pid in self.daemons:
            try: os.kill(pid, signal.SIGTERM)
            except ProcessLookupError: pass
        for process in self.processes:
            if process.poll() is None:
                process.terminate()
                try: process.wait(timeout=4)
                except subprocess.TimeoutExpired: process.kill(); process.wait()
            if process.stdout: process.stdout.close()
            if process.stderr: process.stderr.close()
    def port(self, family=socket.AF_INET):
        with socket.socket(family) as s:
            s.bind(('::1' if family == socket.AF_INET6 else '127.0.0.1', 0))
            return s.getsockname()[1]
    def invoke(self, *args):
        return subprocess.run([str(self.binary), *args], cwd=self.work,
                              capture_output=True, timeout=5)
    def start(self, address='127.0.0.1', daemon=False, extra=(), binary=None):
        port = self.port(socket.AF_INET6 if address == '::1' else socket.AF_INET)
        args = [str(binary or self.binary), '-p', str(port)]
        if not daemon: args += ['-d']
        if address is not None: args += ['-i', address]
        args += list(extra) + ['www']
        process = subprocess.Popen(args, cwd=self.work, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.processes.append(process)
        if daemon:
            stdout, stderr = process.communicate(timeout=5)
            self.assertEqual(process.returncode, 0, stderr)
            self.assertEqual(stdout, b'')
        host = address or '127.0.0.1'
        for _ in range(100):
            try:
                result = self.exchange(port, host, allow_reset=binary is not None)
                if daemon: self.daemons.append(int(result.split('|')[1]))
                return process, port, result
            except OSError: time.sleep(.02)
        self.fail('server not ready')
    def exchange(self, port, host='127.0.0.1', allow_reset=False):
        with socket.create_connection((host, port), timeout=3) as s:
            s.sendall(b'GET / HTTP/1.0\r\n\r\n' if allow_reset else b'X')
            data = bytearray()
            while True:
                try:
                    chunk = s.recv(8192)
                except ConnectionResetError:
                    if allow_reset: break  # production stub closes without reading request
                    raise
                if not chunk: break
                data.extend(chunk)
            return data.decode().strip()
    def test_help_and_invalid_arguments(self):
        self.assertEqual(self.invoke('-h').returncode, 0)
        for args in [[], ['www', 'cgi'], ['-z', 'www'], ['-p'],
                     ['-p', '0', 'www'], ['-p', '65536', 'www'],
                     ['-p', '-1', 'www'], ['-p', 'abc', 'www'],
                     ['-p', ' 80', 'www'], ['-p', '80x', 'www'],
                     ['-p', '9999999999999999999999999', 'www'],
                     ['missing'], ['-c', 'missing', 'www']]:
            with self.subTest(args=args): self.assertNotEqual(self.invoke(*args).returncode, 0)
    def test_ipv4_and_paths(self):
        process, port, result = self.start(extra=('-c', 'cgi', '-l', 'relative.log'))
        fields = result.split('|')
        self.assertEqual(fields[2:5], [str(self.work/'www'), str(self.work/'cgi'), str(self.work/'relative.log')])
        self.assertEqual(int(fields[0]), process.pid)
        self.assertTrue(self.exchange(port).split('|')[5] == '200')
    def test_ipv6(self):
        if not socket.has_ipv6: self.skipTest('IPv6 disabled')
        try: self.port(socket.AF_INET6)
        except OSError: self.skipTest('IPv6 loopback unavailable')
        _, port, _ = self.start(address='::1')
        self.assertTrue(self.exchange(port, '::1').split('|')[5] == '200')
    def test_wildcard_dual_stack(self):
        _, port, _ = self.start(address=None)
        self.assertTrue(self.exchange(port).split('|')[5] == '200')
        if socket.has_ipv6:
            try: self.port(socket.AF_INET6)
            except OSError: return
            self.assertTrue(self.exchange(port, '::1').split('|')[5] == '200')
    def test_occupied_port(self):
        with socket.socket() as s:
            s.bind(('127.0.0.1', 0)); s.listen()
            result = self.invoke('-d', '-i', '127.0.0.1', '-p', str(s.getsockname()[1]), 'www')
            self.assertNotEqual(result.returncode, 0)
    def test_invalid_address(self):
        result = self.invoke('-d', '-i', 'not-an-ip', 'www')
        self.assertNotEqual(result.returncode, 0)
    def test_bad_daemon_log_destination(self):
        result = self.invoke('-l', 'missing-dir/out.log', 'www')
        self.assertNotEqual(result.returncode, 0)
        result = self.invoke('-l', 'www', 'www')
        self.assertNotEqual(result.returncode, 0)
    def test_debug_is_sequential_and_times_out(self):
        process, port, _ = self.start()
        with socket.create_connection(('127.0.0.1', port), timeout=3) as slow:
            time.sleep(.1)
            with socket.create_connection(('127.0.0.1', port), timeout=3) as fast:
                fast.sendall(b'X'); fast.settimeout(.15)
                with self.assertRaises(socket.timeout): fast.recv(8192)
                fast.settimeout(3)
                self.assertTrue(fast.recv(8192).decode().strip().split('|')[5] == '200')
            self.assertIsNotNone(slow.recv(8192))
        self.assertIsNone(process.poll())
    def test_sigterm_during_request(self):
        process, port, _ = self.start()
        with socket.create_connection(('127.0.0.1', port), timeout=3):
            time.sleep(.05)
            process.terminate()
            self.assertEqual(process.wait(timeout=3), 0)
    def test_disconnected_client_and_no_fd_leak(self):
        process, port, _ = self.start()
        before = int(self.exchange(port).split('|')[9])
        for _ in range(30):
            with socket.create_connection(('127.0.0.1', port), timeout=3): pass
        self.assertTrue(self.exchange(port).split('|')[5] == '200')
        self.assertEqual(int(self.exchange(port).split('|')[9]), before)
    def test_daemon_concurrency_paths_and_log(self):
        _, port, result = self.start(daemon=True, extra=('-c', 'cgi', '-l', 'daemon.log'))
        daemon_pid = int(result.split('|')[1])
        self.assertNotEqual(int(result.split('|')[7]), daemon_pid)  # double fork
        self.assertEqual(result.split('|')[6], '/')
        self.assertEqual(result.split('|')[8], '1')
        with socket.create_connection(('127.0.0.1', port), timeout=3) as slow:
            time.sleep(.05)
            start = time.monotonic()
            fast = self.exchange(port)
            self.assertLess(time.monotonic() - start, .7)
            self.assertNotEqual(int(fast.split('|')[0]), daemon_pid)
            slow.sendall(b'X')
            self.assertTrue(slow.recv(8192).decode().strip().split('|')[5] == '200')
        self.assertEqual(result.split('|')[2:5], [str(self.work/'www'), str(self.work/'cgi'), str(self.work/'daemon.log')])
        for _ in range(20): self.exchange(port)
        time.sleep(.4)
        self.assertIn('TEST request 200', (self.work/'daemon.log').read_text())
    def test_real_binary_socket_lifecycle(self):
        # Production binary: accept bootstrap empty response OR implemented HTTP response.
        # Full protocol assertions live in test_team_http.py.
        process, port, response = self.start(binary=ROOT/'sws')
        process.terminate(); process.wait(timeout=3)
        if response:
            self.assertTrue(response.startswith('HTTP/1.0 200 '), response)
        else:
            self.assertIn(b'Function not implemented', process.stderr.read())

if __name__ == '__main__': unittest.main(verbosity=2)

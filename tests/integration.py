"""Behavioral tests against local HTTP and TLS SMTP servers; no internet required."""

import contextlib
import http.server
import os
from pathlib import Path
import signal
import shutil
import socketserver
import ssl
import subprocess
import sys
import tempfile
import threading
import time
import unittest


BINARY = str(Path(sys.argv.pop(1)).resolve())


class HttpHandler(http.server.BaseHTTPRequestHandler):
    status = 200
    delay = 0
    requests = []

    def do_GET(self):
        type(self).requests.append((self.path, self.headers.get("Host")))
        time.sleep(type(self).delay)
        self.send_response(type(self).status)
        self.send_header("Content-Length", "2")
        self.end_headers()
        with contextlib.suppress(BrokenPipeError):
            self.wfile.write(b"OK")

    def log_message(self, *_args):
        pass


class SmtpHandler(socketserver.StreamRequestHandler):
    def finish(self):
        try:
            super().finish()
        finally:
            self.connection.close()

    def handle(self):
        def reply(value):
            self.wfile.write(value + b"\r\n")
            self.wfile.flush()

        self.connection.settimeout(5)
        try:
            if self.server.implicit_tls:
                self.connection = self.server.tls.wrap_socket(self.connection, server_side=True)
                self.rfile = self.connection.makefile("rb")
                self.wfile = self.connection.makefile("wb")
            reply(b"220 localhost fixture")
            while True:
                line = self.rfile.readline()
                if not line:
                    return
                verb = line.split(None, 1)[0].upper()
                if verb in (b"EHLO", b"HELO"):
                    reply(b"250-localhost")
                    reply(b"250 STARTTLS")
                elif verb == b"STARTTLS":
                    reply(b"220 ready for TLS")
                    self.connection = self.server.tls.wrap_socket(self.connection, server_side=True)
                    self.rfile = self.connection.makefile("rb")
                    self.wfile = self.connection.makefile("wb")
                elif verb in (b"MAIL", b"RCPT", b"RSET"):
                    reply(b"250 OK")
                elif verb == b"DATA":
                    reply(b"354 send message")
                    data = bytearray()
                    while True:
                        part = self.rfile.readline()
                        if not part:
                            return
                        if part == b".\r\n":
                            break
                        data.extend(part)
                    with self.server.lock:
                        self.server.attempts += 1
                        if self.server.reject_next:
                            self.server.reject_next = False
                            reply(b"451 temporary fixture error")
                        else:
                            self.server.messages.append(bytes(data).decode())
                            reply(b"250 accepted")
                elif verb == b"QUIT":
                    reply(b"221 bye")
                    return
                else:
                    reply(b"500 unsupported")
        except (ssl.SSLError, OSError):
            # Expected when the client rejects an untrusted test certificate.
            return


class SmtpServer(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True


class IntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="nginxwatch-tests-")
        cls.root = Path(cls.temp.name)
        cert = cls.root / "cert.pem"
        key = cls.root / "key.pem"
        subprocess.run([
            "openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "1",
            "-subj", "/CN=localhost", "-addext",
            "subjectAltName=DNS:localhost,DNS:fixture.invalid,IP:127.0.0.1",
            "-keyout", str(key), "-out", str(cert),
        ], check=True, capture_output=True)
        cls.cert = cert
        cls.tls = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        cls.tls.load_cert_chain(cert, key)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def setUp(self):
        self.work = tempfile.TemporaryDirectory(dir=self.root)
        self.addCleanup(self.work.cleanup)
        self.path = Path(self.work.name)
        self.sites = self.path / "sites"
        self.sites.mkdir()
        self.config = self.path / "monitor.conf"
        self.log = self.path / "events.log"
        HttpHandler.status = 200
        HttpHandler.delay = 0
        HttpHandler.requests = []
        self.http = http.server.ThreadingHTTPServer(("127.0.0.1", 0), HttpHandler)
        threading.Thread(target=self.http.serve_forever, daemon=True).start()
        self.addCleanup(self.http.server_close)
        self.addCleanup(self.http.shutdown)
        self.port = self.http.server_port
        self.write_site()
        self.write_config()

    def write_site(self, host="fixture.invalid", port=None, filename="site.conf"):
        (self.sites / filename).write_text(
            f"server {{ listen {port or self.port}; server_name {host}; }}\n")

    def write_config(self, **settings):
        values = {
            "nginx_dir": self.sites, "check_public": 0, "timeout_sec": 1,
            "interval_sec": 1, "fails_before_down": 2, "log_file": self.log,
        }
        values.update(settings)
        self.config.write_text("".join(f"{k} = {v}\n" for k, v in values.items()))

    def run_monitor(self, *args, expected=0, env=None):
        result = subprocess.run([BINARY, "-c", str(self.config), *args],
                                capture_output=True, text=True, timeout=12, env=env)
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        return result

    def start_smtp(self, implicit_tls=True):
        smtp = SmtpServer(("127.0.0.1", 0), SmtpHandler)
        smtp.tls = self.tls
        smtp.implicit_tls = implicit_tls
        smtp.messages = []
        smtp.attempts = 0
        smtp.reject_next = False
        smtp.lock = threading.Lock()
        threading.Thread(target=smtp.serve_forever, daemon=True).start()
        self.addCleanup(smtp.server_close)
        self.addCleanup(smtp.shutdown)
        return smtp

    def mail_settings(self, smtp):
        return {
            "smtp_url": f"{'smtps' if smtp.implicit_tls else 'smtp'}://localhost:{smtp.server_address[1]}",
            "smtp_starttls": int(not smtp.implicit_tls), "smtp_ca_file": self.cert,
            "mail_from": "monitor@example.test", "mail_to": "recipient@example.test",
        }

    def wait_for(self, condition, message, timeout=10):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if condition():
                return
            time.sleep(0.05)
        self.fail(message)

    def start_monitor(self):
        output = open(self.path / "process.log", "w+")
        self.addCleanup(output.close)
        proc = subprocess.Popen([BINARY, "-c", str(self.config)], stdout=output, stderr=output)

        def stop():
            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait(timeout=5)
        self.addCleanup(stop)
        return proc

    def test_local_bypasses_proxy_and_preserves_host_and_path(self):
        self.write_config(health_path="/health?ready=1")
        env = dict(os.environ, http_proxy="http://127.0.0.1:1", HTTP_PROXY="http://127.0.0.1:1",
                   ALL_PROXY="http://127.0.0.1:1", all_proxy="http://127.0.0.1:1", NO_PROXY="", no_proxy="")
        result = self.run_monitor("--once", env=env)
        self.assertIn("OK 200", result.stdout)
        self.assertEqual(HttpHandler.requests[-1], ("/health?ready=1", f"fixture.invalid:{self.port}"))

    def test_http_statuses(self):
        for status in (200, 301, 401, 404, 500, 502, 503):
            with self.subTest(status=status):
                HttpHandler.status = status
                result = self.run_monitor("--once", expected=int(status >= 500))
                self.assertIn(f"{'FAIL HTTP' if status >= 500 else 'OK'} {status}", result.stdout)

    def test_timeout_and_refused_connection(self):
        HttpHandler.delay = 2
        self.run_monitor("--once", expected=1)
        HttpHandler.delay = 0
        with socketserver.TCPServer(("127.0.0.1", 0), socketserver.BaseRequestHandler) as reservation:
            unused_port = reservation.server_address[1]
        self.write_site(port=unused_port)
        self.run_monitor("--once", expected=1)

    def test_public_only(self):
        self.write_site(host="127.0.0.1")
        self.write_config(check_local=0, check_public=1)
        env = dict(os.environ, NO_PROXY="*", no_proxy="*")
        result = self.run_monitor("--once", env=env)
        self.assertIn("(skipped)", result.stdout)
        self.assertIn("OK 200", result.stdout)

    def test_https_certificate_hostname_and_sni(self):
        names = []
        tls = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        tls.load_cert_chain(self.cert, self.root / "key.pem")
        tls.set_servername_callback(lambda _sock, name, _context: names.append(name))
        self.http.socket = tls.wrap_socket(self.http.socket, server_side=True)
        (self.sites / "site.conf").write_text(
            f"server {{ listen {self.port} ssl; server_name fixture.invalid; }}\n")
        self.write_config(http_ca_file=self.cert)
        self.run_monitor("--once")
        self.assertIn("fixture.invalid", names, "local connection must preserve TLS SNI")
        self.write_config()
        self.run_monitor("--once", expected=1)  # untrusted self-signed certificate
        self.write_config(http_ca_file=self.cert)
        (self.sites / "site.conf").write_text(
            f"server {{ listen {self.port} ssl; server_name wrong.invalid; }}\n")
        self.run_monitor("--once", expected=1)  # trusted CA, wrong hostname

    @unittest.skipUnless(shutil.which("nginx"), "nginx not installed (optional end-to-end test)")
    def test_real_nginx_virtual_host_and_listeners(self):
        ports = []
        reservations = []
        for _ in range(2):
            reservation = socketserver.TCPServer(("127.0.0.1", 0), socketserver.BaseRequestHandler)
            reservations.append(reservation)
            ports.append(reservation.server_address[1])
        for reservation in reservations:
            reservation.server_close()
        (self.sites / "site.conf").write_text(
            f"server {{ listen 127.0.0.1:{ports[0]}; listen 127.0.0.1:{ports[1]} ssl;\n"
            f"server_name fixture.invalid; ssl_certificate {self.cert};\n"
            f"ssl_certificate_key {self.root / 'key.pem'};\n"
            'location = /health { return 200 "healthy"; } location / { return 502; } }\n')
        nginx_config = self.path / "nginx.conf"
        temp_paths = "".join(f"{kind}_temp_path {self.path / kind};\n" for kind in
                             ("client_body", "proxy", "fastcgi", "uwsgi", "scgi"))
        nginx_config.write_text(
            f"pid {self.path / 'nginx.pid'}; error_log {self.path / 'nginx-error.log'};\n"
            f"events {{}} http {{ access_log off; {temp_paths} include {self.sites}/*; }}\n")
        with open(self.path / "nginx-process.log", "w+") as output:
            proc = subprocess.Popen([shutil.which("nginx"), "-c", str(nginx_config),
                                     "-p", str(self.path), "-e", str(self.path / "nginx-error.log"),
                                     "-g", "daemon off;"], stdout=output, stderr=output)
            try:
                self.wait_for(lambda: (self.path / "nginx.pid").exists() or proc.poll() is not None,
                              "nginx did not start")
                self.assertIsNone(proc.poll(), (self.path / "nginx-error.log").read_text())
                self.write_config(http_ca_file=self.cert, health_path="/health")
                result = self.run_monitor("--once")
                self.assertIn("Found 2 site(s)", result.stdout)
                self.assertEqual(result.stdout.count("OK 200"), 2)
                self.write_config(http_ca_file=self.cert)
                result = self.run_monitor("--once", expected=1)
                self.assertEqual(result.stdout.count("FAIL HTTP 502"), 2)
            finally:
                if proc.poll() is None:
                    proc.terminate()
                proc.wait(timeout=5)

    def test_bad_config_and_cli(self):
        for setting in ("timeout_sec = 0", "misspelled = 1", "check_local = 0\ncheck_public = 0",
                        "timeout_sec = 1\ntimeout_sec = 2", "not a setting"):
            with self.subTest(setting=setting):
                self.config.write_text(setting + "\n")
                self.run_monitor("--once", expected=2)
        self.write_config()
        self.run_monitor("--once", "--test-email", expected=2)
        self.run_monitor("-c", expected=2)
        self.run_monitor("--unknown", expected=2)
        self.config.unlink()
        self.run_monitor("--once", expected=2)
        help_result = subprocess.run([BINARY, "--help"], capture_output=True, text=True)
        self.assertEqual(help_result.returncode, 0)
        self.assertIn("Usage:", help_result.stdout)

    def test_discovery_errors_and_symlinks(self):
        site = self.sites / "site.conf"
        site.rename(self.path / "target.conf")
        site.symlink_to(self.path / "target.conf")
        self.run_monitor("--once")
        (self.path / "target.conf").unlink()
        self.run_monitor("--once", expected=2)
        site.unlink()
        self.run_monitor("--once", expected=2)
        site.write_text("server { server_name invalid.example;")
        self.run_monitor("--once", expected=2)
        self.write_site()
        self.write_config(ignore="FIXTURE.INVALID")
        self.run_monitor("--once", expected=2)
        self.write_config(nginx_dir=self.path / "missing")
        self.run_monitor("--once", expected=2)

    def test_encrypted_email_and_certificate_validation(self):
        for implicit in (True, False):
            with self.subTest(implicit_tls=implicit):
                smtp = self.start_smtp(implicit)
                settings = self.mail_settings(smtp)
                self.write_config(**settings)
                self.run_monitor("--test-email")
                self.assertEqual(len(smtp.messages), 1)
                self.assertIn("Subject: nginxwatch test email", smtp.messages[0])
                self.run_monitor("--once")
                self.assertEqual(len(smtp.messages), 1, "--once must not email")
                settings.pop("smtp_ca_file")
                self.write_config(**settings)
                self.run_monitor("--test-email", expected=1)
                self.assertEqual(len(smtp.messages), 1, "untrusted certificate must fail")

    def test_missing_email_settings(self):
        self.run_monitor("--test-email", expected=1)
        self.write_config(smtp_url="smtp://localhost:25", mail_from="a@b", mail_to="c@d")
        self.run_monitor("--test-email", expected=2)

    def test_alert_threshold_retry_recovery_and_shutdown(self):
        smtp = self.start_smtp()
        smtp.reject_next = True
        self.write_config(**self.mail_settings(smtp))
        HttpHandler.status = 502
        proc = self.start_monitor()
        self.wait_for(lambda: len(HttpHandler.requests) >= 1, "first check did not run")
        self.assertEqual(smtp.attempts, 0, "first failure should not alert")
        self.wait_for(lambda: len(smtp.messages) == 1, "DOWN retry was not delivered")
        self.assertEqual(smtp.attempts, 2, "failed email should retry")
        self.assertIn("Subject: DOWN:", smtp.messages[0])
        time.sleep(1.2)
        self.assertEqual(len(smtp.messages), 1, "no duplicate DOWN after successful submission")
        HttpHandler.status = 200
        self.wait_for(lambda: len(smtp.messages) == 2, "recovery was not delivered")
        self.assertIn("Subject: RECOVERED:", smtp.messages[1])
        time.sleep(1.2)
        self.assertEqual(len(smtp.messages), 2, "no duplicate recovery")
        proc.send_signal(signal.SIGTERM)
        self.assertEqual(proc.wait(timeout=5), 0)
        self.assertIn("nginxwatch stopped", self.log.read_text())

    def test_discovery_updates_while_running(self):
        proc = self.start_monitor()
        self.wait_for(lambda: len(HttpHandler.requests) >= 1, "initial check missing")
        self.write_site(host="added.invalid", filename="added.conf")
        self.wait_for(lambda: any(h == f"added.invalid:{self.port}" for _, h in HttpHandler.requests),
                      "new site not discovered")
        (self.sites / "added.conf").unlink()
        time.sleep(1.2)
        before = sum(h == f"added.invalid:{self.port}" for _, h in HttpHandler.requests)
        time.sleep(1.2)
        after = sum(h == f"added.invalid:{self.port}" for _, h in HttpHandler.requests)
        self.assertEqual(before, after, "removed site still checked")
        self.assertIsNone(proc.poll())


if __name__ == "__main__":
    unittest.main(verbosity=2)

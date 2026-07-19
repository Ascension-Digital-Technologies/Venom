#!/usr/bin/env python3
from __future__ import annotations
import argparse, json, os, secrets, shutil, subprocess, sys, threading, webbrowser
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
from console_output import error, info, passed, success, server
from venom_tools.examples import load_example_registry
from venom_tools.process import run_command

class LauncherStepError(RuntimeError):
    def __init__(self, label: str, returncode: int):
        super().__init__(f"{label} failed with exit code {returncode}")
        self.label = label
        self.returncode = returncode


class QuietHandler(SimpleHTTPRequestHandler):
    venom_executable: Path | None = None
    playground_enabled = False
    playground_token = ""
    playground_compile_slots = threading.BoundedSemaphore(2)
    playground_max_source_bytes = 256 * 1024
    playground_max_request_bytes = 320 * 1024

    def log_message(self, _format, *_args):
        pass

    def _json(self, status: int, payload: dict):
        body = json.dumps(payload, separators=(",", ":")).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _expected_origin(self) -> str:
        host = self.headers.get("Host", "")
        return f"http://{host}" if host else ""

    def _playground_request_allowed(self) -> bool:
        if not self.playground_enabled:
            return False
        host = self.headers.get("Host", "")
        if host not in {f"127.0.0.1:{self.server.server_port}", f"localhost:{self.server.server_port}"}:
            return False
        origin = self.headers.get("Origin", "")
        referer = self.headers.get("Referer", "")
        expected = self._expected_origin()
        if origin and origin != expected:
            return False
        if not origin and referer and not referer.startswith(expected + "/"):
            return False
        return True

    def do_GET(self):
        if self.path == "/__venom/playground/session":
            if not self._playground_request_allowed():
                self._json(403, {"ok": False, "error": "playground origin rejected"})
                return
            self._json(200, {
                "ok": True,
                "token": self.playground_token,
                "limits": {
                    "sourceBytes": self.playground_max_source_bytes,
                    "inputBytes": 64 * 1024,
                    "resultBytes": 256 * 1024,
                    "consoleEvents": 128,
                    "consoleBytes": 128 * 1024,
                    "executionMs": 2000,
                    "heapBytes": 8 * 1024 * 1024,
                    "stackBytes": 256 * 1024,
                },
            })
            return
        super().do_GET()

    def do_POST(self):
        if self.path != "/__venom/playground/compile":
            self._json(404, {"ok": False, "error": "not found"})
            return
        if not self._playground_request_allowed():
            self._json(403, {"ok": False, "error": "playground origin rejected"})
            return
        if not secrets.compare_digest(self.headers.get("X-Venom-Playground-Token", ""), self.playground_token):
            self._json(403, {"ok": False, "error": "playground capability token rejected"})
            return
        if self.headers.get("Content-Type", "").split(";", 1)[0].strip().lower() != "application/json":
            self._json(415, {"ok": False, "error": "application/json is required"})
            return
        if not self.playground_compile_slots.acquire(blocking=False):
            self._json(429, {"ok": False, "error": "compiler concurrency limit reached"})
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if length <= 0 or length > self.playground_max_request_bytes:
                self._json(413, {"ok": False, "error": "playground request is too large"})
                return
            request = json.loads(self.rfile.read(length).decode("utf-8"))
            source = request.get("source", "")
            if not isinstance(source, str) or not source.strip():
                self._json(400, {"ok": False, "error": "source is required"})
                return
            if len(source.encode("utf-8")) > self.playground_max_source_bytes:
                self._json(413, {"ok": False, "error": "source exceeds the 256 KiB limit"})
                return
            if self.venom_executable is None:
                raise RuntimeError("Venom compiler is unavailable")
            safe_env = {key: os.environ[key] for key in ("PATH", "SYSTEMROOT", "WINDIR", "TEMP", "TMP") if key in os.environ}
            safe_env.update({"VENOM_PLAYGROUND_COMPILER": "1", "NO_COLOR": "1"})
            proc = subprocess.run(
                [str(self.venom_executable), "compile-snippet"],
                input=source.encode("utf-8"), stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, timeout=10, env=safe_env,
                creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
            stdout_text = proc.stdout.decode("utf-8", errors="replace")
            stderr_text = proc.stderr.decode("utf-8", errors="replace")
            if proc.returncode:
                message = (stderr_text or stdout_text or "QuickJS compilation failed").strip()
                self._json(400, {"ok": False, "error": message})
                return
            self._json(200, json.loads(stdout_text))
        except subprocess.TimeoutExpired:
            self._json(408, {"ok": False, "error": "QuickJS compilation timed out"})
        except Exception as exc:
            self._json(500, {"ok": False, "error": str(exc)})
        finally:
            self.playground_compile_slots.release()


def run(cmd, cwd, label, visible=False):
    info(f'{label}...')
    result = run_command(cmd, cwd=Path(cwd), stream=visible)
    if not result.passed:
        if not visible and result.output:
            print(result.output.rstrip(), file=sys.stderr)
        raise LauncherStepError(label, result.returncode)
    passed(label)


def main():
    root=Path(__file__).resolve().parents[1]
    registry=load_example_registry(root)
    lookup=registry.lookup()
    ap=argparse.ArgumentParser(description='Build Venom and launch a named production example.')
    ap.add_argument('example', choices=sorted(lookup), help='semantic example name')
    ap.add_argument('--profile', choices=[registry.build_profile])
    ap.add_argument('--port', type=int)
    ap.add_argument('--no-open', action='store_true')
    ap.add_argument('--build-dir', default='build')
    ns=ap.parse_args()
    spec=lookup[ns.example]
    name, default_port=spec.directory, spec.port
    profile=registry.build_profile
    port=ns.port or default_port
    build=(root/ns.build_dir).resolve()
    site=root/'examples'/name
    dist=root/f'dist-{name}-{profile}'

    run(['cmake','-S',root,'-B',build,'-DCMAKE_BUILD_TYPE=Release'], root, 'Configure native compiler')
    run(['cmake','--build',build,'--config','Release','--parallel'], root, 'Build native compiler')
    candidates=[build/'venom',build/'Release'/'venom.exe',build/'venom.exe']
    venom=next((p for p in candidates if p.is_file()),None)
    if venom is None: raise SystemExit(f'venom executable not found under {build}')
    if dist.exists(): shutil.rmtree(dist)

    run([venom,'build',site,'--out',dist,'--profile',profile], root, f'Build {name}', visible=True)
    if profile=='prod':
        run([sys.executable,root/'tools/check_production_leaks.py',dist], root, 'Production leak scan')
        run([venom,'verify',dist,'--target','browser'], root, 'Package integrity verification')
        run([venom,'verify-runtime',dist,'--require-real-engine'], root, 'QuickJS runtime verification')
        if spec.chrome_extension:
            run([sys.executable, root/'tools/verify_chrome_extension_store.py', dist,
                 '--report', root/'artifacts/chrome-extension-store-readiness.json'],
                root, 'Chrome Web Store readiness')

    if spec.chrome_extension:
        print()
        success(f'{name} is ready for Load unpacked')
        info(f'Extension directory: {dist}')
        info('Open chrome://extensions, enable Developer mode, and choose Load unpacked.')
        if not ns.no_open:
            try:
                if os.name == 'nt':
                    os.startfile(str(dist))
                webbrowser.open('chrome://extensions/')
            except Exception:
                pass
        return 0

    url=f'http://127.0.0.1:{port}/'
    print()
    success(f'{name} is ready')
    server(url)
    info('Press Ctrl+C to stop the server.')
    print()
    if not ns.no_open:
        try: webbrowser.open(url)
        except Exception: pass
    os.chdir(dist)
    QuietHandler.venom_executable = venom
    QuietHandler.playground_enabled = spec.playground
    QuietHandler.playground_token = secrets.token_urlsafe(32) if spec.playground else ""
    ThreadingHTTPServer(('127.0.0.1',port),QuietHandler).serve_forever()

if __name__=='__main__':
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print()
        info('Server stopped.')
        raise SystemExit(0)
    except LauncherStepError as exc:
        error(f'{exc.label} failed; launcher stopped cleanly (exit code {exc.returncode}).')
        raise SystemExit(exc.returncode)
    except Exception as exc:
        error(f'Launcher failed: {exc}')
        raise SystemExit(1)

#!/usr/bin/env python3
"""Venom development server with content-addressed caching and live reload."""
from __future__ import annotations

import argparse
import hashlib
import http.server
import json
import os
import pathlib
import shutil
import socketserver
import subprocess
import tempfile
import threading
import time
import urllib.parse
import webbrowser

RELOAD_SNIPPET = b'''<script>(function(){let v='';function apply(s){if(!s)return;if(s.error){console.error('[venom] build failed',s.error);return;}if(v&&s.version!==v)location.reload();v=s.version||v;}if('EventSource'in window){const e=new EventSource('/__venom_events');e.onmessage=x=>{try{apply(JSON.parse(x.data))}catch(_){}};e.onerror=()=>{};}else{async function p(){try{apply(await(await fetch('/__venom_status',{cache:'no-store'})).json())}catch(_){}setTimeout(p,1000)}p()}})();</script>'''

SKIP_NAMES = {'.git', 'build', 'dist', 'dist-dev', 'dist-prod', 'node_modules', '__pycache__'}


class State:
    def __init__(self) -> None:
        self.version = '0'
        self.error = ''
        self.building = False
        self.duration_ms = 0
        self.changed: list[str] = []
        self.lock = threading.Lock()
        self.changed_condition = threading.Condition(self.lock)

    def success(self, duration_ms: int, changed: list[str]) -> None:
        with self.changed_condition:
            self.version = str(time.time_ns())
            self.error = ''
            self.building = False
            self.duration_ms = duration_ms
            self.changed = changed
            self.changed_condition.notify_all()

    def fail(self, message: str, duration_ms: int) -> None:
        with self.changed_condition:
            self.error = message
            self.building = False
            self.duration_ms = duration_ms
            self.changed_condition.notify_all()

    def start(self, changed: list[str]) -> None:
        with self.changed_condition:
            self.building = True
            self.changed = changed
            self.changed_condition.notify_all()

    def snapshot(self) -> dict[str, object]:
        with self.lock:
            return {
                'version': self.version,
                'error': self.error,
                'building': self.building,
                'durationMs': self.duration_ms,
                'changed': list(self.changed),
            }


def file_manifest(root: pathlib.Path) -> dict[str, str]:
    result: dict[str, str] = {}
    if not root.exists():
        return result
    for path in sorted(root.rglob('*')):
        if not path.is_file() or any(part in SKIP_NAMES for part in path.relative_to(root).parts):
            continue
        try:
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            result[path.relative_to(root).as_posix()] = digest
        except OSError:
            continue
    return result


def manifest_digest(manifest: dict[str, str]) -> str:
    h = hashlib.sha256()
    for name, digest in sorted(manifest.items()):
        h.update(name.encode('utf-8'))
        h.update(b'\0')
        h.update(digest.encode('ascii'))
        h.update(b'\n')
    return h.hexdigest()


def executable_digest(path: pathlib.Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def cache_path(dist: pathlib.Path) -> pathlib.Path:
    return dist / 'build' / '.venom-dev-cache.json'


def cache_key(venom: pathlib.Path, site_manifest: dict[str, str]) -> dict[str, object]:
    return {
        'schema': 2,
        'profile': 'dev',
        'siteDigest': manifest_digest(site_manifest),
        'venomDigest': executable_digest(venom),
    }


def cache_matches(venom: pathlib.Path, manifest: dict[str, str], dist: pathlib.Path) -> bool:
    try:
        return (
            (dist / 'index.html').is_file()
            and json.loads(cache_path(dist).read_text(encoding='utf-8')) == cache_key(venom, manifest)
        )
    except (OSError, ValueError, TypeError):
        return False


def write_cache(venom: pathlib.Path, manifest: dict[str, str], dist: pathlib.Path) -> None:
    path = cache_path(dist)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(cache_key(venom, manifest), indent=2, sort_keys=True) + '\n', encoding='utf-8')


def changed_files(before: dict[str, str], after: dict[str, str]) -> list[str]:
    return sorted(name for name in set(before) | set(after) if before.get(name) != after.get(name))


def atomic_swap(staged: pathlib.Path, dist: pathlib.Path) -> None:
    backup = dist.with_name(dist.name + '.venom-old')
    if backup.exists():
        shutil.rmtree(backup, ignore_errors=True)
    if dist.exists():
        os.replace(dist, backup)
    try:
        os.replace(staged, dist)
    except Exception:
        if backup.exists() and not dist.exists():
            os.replace(backup, dist)
        raise
    shutil.rmtree(backup, ignore_errors=True)


def run_build(venom: pathlib.Path, site: pathlib.Path, dist: pathlib.Path, manifest: dict[str, str]) -> tuple[bool, str]:
    dist.parent.mkdir(parents=True, exist_ok=True)
    staged = pathlib.Path(tempfile.mkdtemp(prefix=dist.name + '.venom-stage-', dir=dist.parent))
    cmd = [str(venom), 'build', str(site), '--out', str(staged), '--profile', 'dev']
    print('[venom] build:', ' '.join(cmd), flush=True)
    try:
        cp = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if cp.stdout:
            print(cp.stdout, end='' if cp.stdout.endswith('\n') else '\n')
        if cp.returncode != 0:
            return False, cp.stdout or f'build exited with code {cp.returncode}'
        write_cache(venom, manifest, staged)
        atomic_swap(staged, dist)
        return True, cp.stdout
    finally:
        if staged.exists():
            shutil.rmtree(staged, ignore_errors=True)


def watcher(state: State, venom: pathlib.Path, site: pathlib.Path, dist: pathlib.Path, interval: float, debounce: float, initial: dict[str, str]) -> None:
    last_successful = dict(initial)
    observed = dict(initial)
    changed_at = 0.0
    while True:
        current = file_manifest(site)
        if current != observed:
            observed = current
            changed_at = time.monotonic()
        if observed != last_successful and changed_at and time.monotonic() - changed_at >= debounce:
            changed = changed_files(last_successful, observed)
            state.start(changed)
            started = time.perf_counter()
            ok, output = run_build(venom, site, dist, observed)
            duration = round((time.perf_counter() - started) * 1000)
            if ok:
                last_successful = dict(observed)
                state.success(duration, changed)
                print(f'[venom] rebuilt {len(changed)} file(s) in {duration} ms', flush=True)
            else:
                state.fail((output or 'build failed')[-8000:], duration)
                print('[venom] build failed; continuing to serve the last successful output', flush=True)
            changed_at = 0.0
        time.sleep(interval)


class Handler(http.server.SimpleHTTPRequestHandler):
    server_version = 'VenomDev/2.0'

    def translate_path(self, path: str) -> str:
        clean = urllib.parse.unquote(path.split('?', 1)[0].split('#', 1)[0])
        parts = [part for part in pathlib.PurePosixPath(clean).parts if part not in {'/', '.', '..'}]
        target = self.server.dist
        for part in parts:
            target = target / part
        return str(target)

    def _send_json(self, value: object) -> None:
        data = json.dumps(value, separators=(',', ':')).encode('utf-8')
        self.send_response(200)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.send_header('Cache-Control', 'no-store')
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self) -> None:
        clean = self.path.split('?', 1)[0]
        if clean == '/__venom_status':
            self._send_json(self.server.state.snapshot())
            return
        if clean == '/__venom_events':
            self.send_response(200)
            self.send_header('Content-Type', 'text/event-stream')
            self.send_header('Cache-Control', 'no-store')
            self.send_header('Connection', 'keep-alive')
            self.end_headers()
            last = ''
            deadline = time.monotonic() + 25
            try:
                while time.monotonic() < deadline:
                    snap = self.server.state.snapshot()
                    encoded = json.dumps(snap, separators=(',', ':'))
                    if encoded != last:
                        self.wfile.write(('data:' + encoded + '\n\n').encode('utf-8'))
                        self.wfile.flush()
                        last = encoded
                    with self.server.state.changed_condition:
                        self.server.state.changed_condition.wait(timeout=5)
                    self.wfile.write(b':keepalive\n\n')
                    self.wfile.flush()
            except (BrokenPipeError, ConnectionResetError):
                pass
            return
        target = pathlib.Path(self.translate_path(clean))
        if target.is_dir():
            target = target / 'index.html'
        if target.is_file() and target.suffix.lower() == '.html':
            try:
                data = target.read_bytes()
            except OSError:
                self.send_error(404)
                return
            marker = b'</body>'
            data = data.replace(marker, RELOAD_SNIPPET + marker) if marker in data else data + RELOAD_SNIPPET
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.send_header('Cache-Control', 'no-store')
            self.send_header('Content-Length', str(len(data)))
            self.end_headers()
            self.wfile.write(data)
            return
        super().do_GET()


class Server(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--venom', required=True)
    parser.add_argument('--site', required=True)
    parser.add_argument('--dist', required=True)
    parser.add_argument('--host', default='127.0.0.1')
    parser.add_argument('--port', type=int, default=8080)
    parser.add_argument('--open', action='store_true')
    parser.add_argument('--no-watch', action='store_true')
    parser.add_argument('--interval', type=float, default=0.20)
    parser.add_argument('--debounce', type=float, default=0.25)
    ns = parser.parse_args()

    venom = pathlib.Path(ns.venom).resolve()
    site = pathlib.Path(ns.site).resolve()
    dist = pathlib.Path(ns.dist).resolve()
    if not venom.is_file():
        raise SystemExit(f'venom executable not found: {venom}')
    if not site.is_dir():
        raise SystemExit(f'site directory not found: {site}')

    initial = file_manifest(site)
    if cache_matches(venom, initial, dist):
        print('[venom] dev cache hit: existing output is current', flush=True)
    else:
        ok, _ = run_build(venom, site, dist, initial)
        if not ok:
            return 1

    state = State()
    state.success(0, [])
    if not ns.no_watch:
        threading.Thread(
            target=watcher,
            args=(state, venom, site, dist, ns.interval, ns.debounce, initial),
            daemon=True,
        ).start()

    server = Server((ns.host, ns.port), Handler)
    server.state = state
    server.dist = dist
    url = f'http://{ns.host}:{ns.port}/'
    print(f'[venom] dev server: {url}', flush=True)
    print('[venom] watching for changes' if not ns.no_watch else '[venom] watch disabled', flush=True)
    if ns.open:
        threading.Timer(0.4, lambda: webbrowser.open(url)).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print('\n[venom] stopped')
    finally:
        server.server_close()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

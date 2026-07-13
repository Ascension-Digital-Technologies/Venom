#!/usr/bin/env python3
"""Run Venom distributions in real browsers and emit a machine-readable report."""
from __future__ import annotations

import argparse
import functools
import hashlib
import http.server
import json
import os
from pathlib import Path
import socketserver
import subprocess
import sys
import threading
import time

SCHEMA_VERSION = 1
SUPPORTED_BROWSERS = ("chromium", "firefox", "webkit")


class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, format: str, *args: object) -> None:
        pass

    def end_headers(self) -> None:
        self.send_header("Cache-Control", "no-store")
        super().end_headers()


def tree_digest(root: Path) -> str:
    digest = hashlib.sha256()
    for path in sorted(p for p in root.rglob("*") if p.is_file()):
        rel = path.relative_to(root).as_posix().encode("utf-8")
        digest.update(len(rel).to_bytes(4, "big"))
        digest.update(rel)
        digest.update(path.read_bytes())
    return digest.hexdigest()


def start_server(root: Path, host: str, port: int):
    handler = functools.partial(QuietHandler, directory=str(root))
    server = socketserver.ThreadingTCPServer((host, port), handler)
    server.daemon_threads = True
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    return server, server.server_address[1]


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate a generated Venom distribution in real Playwright browsers.")
    parser.add_argument("dist", type=Path)
    parser.add_argument("--browser", choices=(*SUPPORTED_BROWSERS, "all"), default="chromium")
    parser.add_argument("--manifest", type=Path, help="Fixture manifest; defaults to <source>/venom.browser.json when supplied explicitly")
    parser.add_argument("--node", default="node")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=0)
    parser.add_argument("--format", choices=("text", "json"), default="text")
    parser.add_argument("--json-out", type=Path)
    parser.add_argument("--timeout", type=int, default=90)
    args = parser.parse_args()

    dist = args.dist.resolve()
    if not (dist / "index.html").is_file():
        print(f"browser validation failed: {dist / 'index.html'} is missing", file=sys.stderr)
        return 60

    root = Path(__file__).resolve().parents[1]
    runner = root / "tests" / "runtime" / "browser-e2e-runner.mjs"
    manifest = (args.manifest.resolve() if args.manifest else None)
    if manifest is None or not manifest.is_file():
        print("browser validation failed: --manifest must identify a venom.browser.json fixture manifest", file=sys.stderr)
        return 60
    try:
        manifest_data = json.loads(manifest.read_text(encoding="utf-8"))
        if manifest_data.get("schema_version") != 1 or not manifest_data.get("id") or not isinstance(manifest_data.get("scenarios"), list):
            raise ValueError("invalid schema")
    except Exception as exc:
        print(f"browser validation failed: invalid manifest: {exc}", file=sys.stderr)
        return 60
    browsers = SUPPORTED_BROWSERS if args.browser == "all" else (args.browser,)
    server, actual_port = start_server(dist, args.host, args.port)
    url = f"http://{args.host}:{actual_port}/index.html"
    results: list[dict] = []
    started = time.time()
    try:
        for browser in browsers:
            process = subprocess.run(
                [args.node, str(runner), browser, url, str(manifest)],
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                timeout=args.timeout,
                env={**os.environ, "CI": os.environ.get("CI", "")},
            )
            payload = None
            for line in reversed(process.stdout.splitlines()):
                try:
                    payload = json.loads(line)
                    break
                except json.JSONDecodeError:
                    continue
            if not isinstance(payload, dict):
                payload = {
                    "browser": browser,
                    "passed": False,
                    "checks": [],
                    "error": "browser runner did not emit JSON",
                    "stdout": process.stdout,
                    "stderr": process.stderr,
                }
            payload["runner_exit_code"] = process.returncode
            if process.stderr.strip():
                payload["runner_stderr"] = process.stderr.strip()
            results.append(payload)
    except subprocess.TimeoutExpired as exc:
        results.append({
            "browser": "unknown",
            "passed": False,
            "checks": [],
            "error": f"browser validation exceeded {args.timeout}s",
            "stdout": exc.stdout or "",
            "stderr": exc.stderr or "",
        })
    finally:
        server.shutdown()
        server.server_close()

    passed = bool(results) and all(item.get("passed") is True for item in results)
    report = {
        "schema_version": SCHEMA_VERSION,
        "passed": passed,
        "dist": str(dist),
        "dist_sha256": tree_digest(dist),
        "fixture_id": manifest_data["id"],
        "fixture_profile": manifest_data.get("profile", "unspecified"),
        "evidence_level": manifest_data.get("evidence_level", "behavioral"),
        "claims": manifest_data.get("claims", []),
        "support_tier": manifest_data.get("support_tier", "probe"),
        "framework": manifest_data.get("framework"),
        "manifest": str(manifest),
        "manifest_sha256": hashlib.sha256(manifest.read_bytes()).hexdigest(),
        "browsers_requested": list(browsers),
        "duration_ms": int((time.time() - started) * 1000),
        "results": results,
    }
    encoded = json.dumps(report, indent=2, sort_keys=True)
    if args.json_out:
        args.json_out.parent.mkdir(parents=True, exist_ok=True)
        args.json_out.write_text(encoded + "\n", encoding="utf-8")
    if args.format == "json":
        print(encoded)
    else:
        print("Venom browser validation report")
        print(f"Distribution: {dist}")
        print(f"Fixture: {manifest_data["id"]}")
        print(f"Result: {'PASS' if passed else 'FAIL'}")
        for item in results:
            print(f"\n[{ 'PASS' if item.get('passed') else 'FAIL' }] {item.get('browser', 'unknown')}")
            for check in item.get("checks", []):
                print(f"  [{ 'PASS' if check.get('passed') else 'FAIL' }] {check.get('id')}: {check.get('detail', '')}")
            if item.get("error"):
                print(f"  {item['error']}")
    return 0 if passed else 60


if __name__ == "__main__":
    raise SystemExit(main())

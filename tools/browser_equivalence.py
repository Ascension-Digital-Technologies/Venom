#!/usr/bin/env python3
"""Compare an original website and its Venom distribution in real browsers."""
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


def start_server(root: Path, host: str):
    handler = functools.partial(QuietHandler, directory=str(root))
    server = socketserver.ThreadingTCPServer((host, 0), handler)
    server.daemon_threads = True
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    return server, server.server_address[1]


def load_runner_payload(process: subprocess.CompletedProcess[str], browser: str) -> dict:
    for line in reversed(process.stdout.splitlines()):
        try:
            payload = json.loads(line)
            if isinstance(payload, dict):
                payload["runner_exit_code"] = process.returncode
                if process.stderr.strip():
                    payload["runner_stderr"] = process.stderr.strip()
                return payload
        except json.JSONDecodeError:
            continue
    return {
        "browser": browser,
        "passed": False,
        "error": "equivalence runner did not emit JSON",
        "stdout": process.stdout,
        "stderr": process.stderr,
        "runner_exit_code": process.returncode,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Compare original and Venom-built website behavior in Playwright browsers.")
    parser.add_argument("source", type=Path)
    parser.add_argument("dist", type=Path)
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--browser", choices=(*SUPPORTED_BROWSERS, "all"), default="chromium")
    parser.add_argument("--node", default="node")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--timeout", type=int, default=120)
    parser.add_argument("--format", choices=("text", "json"), default="text")
    parser.add_argument("--json-out", type=Path)
    args = parser.parse_args()

    source = args.source.resolve()
    dist = args.dist.resolve()
    manifest = (args.manifest or source / "venom.browser.json").resolve()
    for label, root in (("source", source), ("dist", dist)):
        if not (root / "index.html").is_file():
            print(f"browser equivalence failed: {label} index.html is missing under {root}", file=sys.stderr)
            return 60
    if not manifest.is_file():
        print(f"browser equivalence failed: manifest is missing: {manifest}", file=sys.stderr)
        return 60
    try:
        manifest_data = json.loads(manifest.read_text(encoding="utf-8"))
        if manifest_data.get("schema_version") != 1 or not manifest_data.get("id") or not isinstance(manifest_data.get("scenarios"), list):
            raise ValueError("expected schema_version=1, id, and scenarios[]")
    except Exception as exc:
        print(f"browser equivalence failed: invalid manifest: {exc}", file=sys.stderr)
        return 60

    repo = Path(__file__).resolve().parents[1]
    runner = repo / "tests/runtime/browser-equivalence-runner.mjs"
    browsers = SUPPORTED_BROWSERS if args.browser == "all" else (args.browser,)
    source_server, source_port = start_server(source, args.host)
    dist_server, dist_port = start_server(dist, args.host)
    source_url = f"http://{args.host}:{source_port}/index.html"
    dist_url = f"http://{args.host}:{dist_port}/index.html"
    started = time.time()
    results: list[dict] = []
    try:
        for browser in browsers:
            process = subprocess.run(
                [args.node, str(runner), browser, source_url, dist_url, str(manifest)],
                text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                timeout=args.timeout, env={**os.environ, "CI": os.environ.get("CI", "")},
            )
            results.append(load_runner_payload(process, browser))
    except subprocess.TimeoutExpired as exc:
        results.append({"browser": "unknown", "passed": False, "error": f"equivalence exceeded {args.timeout}s", "stdout": exc.stdout or "", "stderr": exc.stderr or ""})
    finally:
        source_server.shutdown(); source_server.server_close()
        dist_server.shutdown(); dist_server.server_close()

    passed = bool(results) and all(item.get("passed") is True for item in results)
    report = {
        "schema_version": SCHEMA_VERSION,
        "passed": passed,
        "fixture_id": manifest_data["id"],
        "source": str(source),
        "dist": str(dist),
        "source_sha256": tree_digest(source),
        "dist_sha256": tree_digest(dist),
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
        print("Venom browser equivalence report")
        print(f"Fixture: {manifest_data['id']}")
        print(f"Source: {source}")
        print(f"Distribution: {dist}")
        print(f"Result: {'PASS' if passed else 'FAIL'}")
        for item in results:
            print(f"\n[{'PASS' if item.get('passed') else 'FAIL'}] {item.get('browser', 'unknown')}")
            for scenario in item.get("scenarios", []):
                print(f"  [{'PASS' if scenario.get('passed') else 'FAIL'}] {scenario.get('id')}")
                for comparison in scenario.get("comparisons", []):
                    state = comparison.get("source_expected") and comparison.get("dist_expected") and comparison.get("equivalent")
                    print(f"    [{'PASS' if state else 'FAIL'}] {comparison.get('id')}: source={comparison.get('source')!r} dist={comparison.get('dist')!r}")
            if item.get("error"):
                print(f"  {item['error']}")
    return 0 if passed else 60


if __name__ == "__main__":
    raise SystemExit(main())

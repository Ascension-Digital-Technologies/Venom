#!/usr/bin/env python3
"""Run a generated Venom distribution in real browsers and emit a report."""
from __future__ import annotations

import argparse
import functools
import hashlib
import http.server
import json
from pathlib import Path
import socketserver
import sys
import threading
import time
from typing import Any

SCHEMA_VERSION = 2
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


def check_value(page, check: dict[str, Any]) -> bool:
    selector = check.get("selector")
    kind = check.get("type")
    if kind == "exists":
        return page.locator(selector).count() > 0
    if kind == "text":
        return (page.text_content(selector) or "") == str(check.get("equals", ""))
    if kind == "contains-text":
        return str(check.get("contains", "")) in (page.text_content(selector) or "")
    if kind == "attribute":
        value = page.get_attribute(selector, check["name"])
        if check.get("exists") is True:
            return value is not None
        if check.get("exists") is False:
            return value is None
        return value == str(check.get("equals", ""))
    if kind == "class":
        return str(check.get("contains", "")) in (page.get_attribute(selector, "class") or "").split()
    if kind == "url-path":
        return page.url.split("?", 1)[0].endswith(str(check.get("equals", "")))
    if kind == "not-text":
        return (page.text_content(selector) or "") != str(check.get("equals", ""))
    if kind == "evaluate":
        return bool(page.evaluate(str(check["expression"])))
    raise ValueError(f"unsupported check type: {kind}")


def run_browser(playwright, browser_name: str, base_url: str, manifest: dict[str, Any], chromium_executable: str | None = None) -> dict[str, Any]:
    result: dict[str, Any] = {
        "browser": browser_name,
        "fixture": manifest.get("id", "unnamed"),
        "url": base_url,
        "passed": False,
        "checks": [],
        "console_errors": [],
        "page_errors": [],
        "request_failures": [],
        "duration_ms": 0,
    }
    started = time.perf_counter()
    browser = None

    def record(identifier: str, passed: bool, detail: str = "") -> None:
        result["checks"].append({"id": identifier, "passed": bool(passed), "detail": detail})
        if not passed:
            raise RuntimeError(f"{identifier}: {detail}")

    try:
        browser_type = getattr(playwright, browser_name)
        launch_options: dict[str, Any] = {"headless": True}
        if browser_name == "chromium" and chromium_executable:
            launch_options["executable_path"] = chromium_executable
        browser = browser_type.launch(**launch_options)
        context = browser.new_context(service_workers="block")
        page = context.new_page()
        page.on("console", lambda message: result["console_errors"].append(message.text) if message.type == "error" else None)
        page.on("pageerror", lambda error: result["page_errors"].append(str(error)))
        page.on("requestfailed", lambda request: result["request_failures"].append(f"{request.method} {request.url}: {request.failure}"))

        for scenario in manifest.get("scenarios", []):
            scenario_id = scenario["id"]
            target = base_url.rstrip("/") + "/" + str(scenario.get("path", "/")).lstrip("/")
            timeout = int(scenario.get("timeout_ms", 30_000))
            response = page.goto(target, wait_until="networkidle", timeout=timeout)
            record(f"{scenario_id}.navigation", bool(response and response.ok), f"{response.status if response else 'no response'}")

            wait_for = scenario.get("wait_for", {})
            if wait_for.get("selector"):
                page.wait_for_selector(wait_for["selector"], timeout=timeout)
            if wait_for.get("expression"):
                page.wait_for_function(wait_for["expression"], timeout=timeout)

            for action in scenario.get("actions", []):
                kind = action["type"]
                if kind == "click":
                    page.click(action["selector"])
                elif kind == "fill":
                    page.fill(action["selector"], str(action.get("value", "")))
                elif kind == "select":
                    page.select_option(action["selector"], str(action.get("value", "")))
                elif kind == "evaluate":
                    page.evaluate(action["expression"])
                elif kind == "wait-for-function":
                    page.wait_for_function(action["expression"], timeout=int(action.get("timeout_ms", timeout)))
                else:
                    raise ValueError(f"unsupported action type: {kind}")

            for check in scenario.get("checks", []):
                record(f"{scenario_id}.{check['id']}", check_value(page, check), json.dumps(check, sort_keys=True))

        record("runtime.page-errors", not result["page_errors"], "\n".join(result["page_errors"]))
        record("runtime.console-errors", not result["console_errors"], "\n".join(result["console_errors"]))
        record("runtime.request-failures", not result["request_failures"], "\n".join(result["request_failures"]))
        result["passed"] = True
    except Exception as exc:
        result["error"] = str(exc)
    finally:
        result["duration_ms"] = int((time.perf_counter() - started) * 1000)
        if browser:
            browser.close()
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate a generated Venom distribution in real Playwright browsers.")
    parser.add_argument("dist", type=Path)
    parser.add_argument("--browser", choices=(*SUPPORTED_BROWSERS, "all"), default="chromium")
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=0)
    parser.add_argument("--format", choices=("text", "json"), default="text")
    parser.add_argument("--json-out", type=Path)
    parser.add_argument("--chromium-executable", help="Use a system Chromium executable instead of Playwright's managed browser")
    args = parser.parse_args()

    dist = args.dist.resolve()
    if not (dist / "index.html").is_file():
        print(f"browser validation failed: {dist / 'index.html'} is missing", file=sys.stderr)
        return 60

    manifest = args.manifest.resolve()
    try:
        manifest_data = json.loads(manifest.read_text(encoding="utf-8"))
        if manifest_data.get("schema_version") not in (1, 2) or not manifest_data.get("id") or not isinstance(manifest_data.get("scenarios"), list):
            raise ValueError("expected browser manifest schema_version 1 or 2")
    except Exception as exc:
        print(f"browser validation failed: invalid manifest: {exc}", file=sys.stderr)
        return 60

    try:
        from playwright.sync_api import sync_playwright
    except ImportError:
        print("browser validation failed: install Python Playwright with 'python -m pip install playwright'", file=sys.stderr)
        return 60

    browsers = SUPPORTED_BROWSERS if args.browser == "all" else (args.browser,)
    server, actual_port = start_server(dist, args.host, args.port)
    url = f"http://{args.host}:{actual_port}/"
    started = time.time()
    try:
        with sync_playwright() as playwright:
            results = [run_browser(playwright, browser, url, manifest_data, args.chromium_executable) for browser in browsers]
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
        "evidence_level": manifest_data.get("evidence_level", "runtime-qualification"),
        "support_tier": manifest_data.get("support_tier", "probe"),
        "manifest": str(manifest),
        "manifest_sha256": hashlib.sha256(manifest.read_bytes()).hexdigest(),
        "claims": manifest_data.get("claims", []),
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
        print("Venom browser runtime qualification")
        print(f"Distribution: {dist}")
        print(f"Result: {'PASS' if passed else 'FAIL'}")
        for item in results:
            print(f"\n[{'PASS' if item.get('passed') else 'FAIL'}] {item['browser']}")
            for check in item.get("checks", []):
                print(f"  [{'PASS' if check['passed'] else 'FAIL'}] {check['id']}")
            if item.get("error"):
                print(f"  {item['error']}")
    return 0 if passed else 60


if __name__ == "__main__":
    raise SystemExit(main())

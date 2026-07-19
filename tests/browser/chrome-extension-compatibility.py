#!/usr/bin/env python3
"""Real Chromium smoke coverage for core Manifest V3 execution contexts."""
from __future__ import annotations

import argparse
import contextlib
import http.server
import os
from pathlib import Path
import shutil
import socketserver
import subprocess
import sys
import tempfile
import threading
import time


def ensure_display() -> None:
    if os.name == "nt" or os.environ.get("DISPLAY") or os.environ.get("VENOM_CHROME_FIXTURE_CHILD"):
        return
    xvfb = shutil.which("xvfb-run")
    if xvfb:
        env = dict(os.environ)
        env["VENOM_CHROME_FIXTURE_CHILD"] = "1"
        raise SystemExit(subprocess.call([xvfb, "-a", sys.executable, *sys.argv], env=env))


def chromium_path(explicit: str | None) -> str:
    candidates = [explicit, os.environ.get("VENOM_CHROMIUM"), shutil.which("chromium"),
                  shutil.which("chromium-browser"), shutil.which("google-chrome")]
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return str(Path(candidate))
    raise RuntimeError("Chromium/Chrome executable not found; set VENOM_CHROMIUM")


class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *_args) -> None:
        pass


@contextlib.contextmanager
def local_page_server():
    with tempfile.TemporaryDirectory(prefix="venom-extension-page-") as root:
        Path(root, "index.html").write_text("<!doctype html><title>fixture</title><main>fixture</main>", encoding="utf-8")
        handler = lambda *args, **kwargs: QuietHandler(*args, directory=root, **kwargs)
        with socketserver.TCPServer(("127.0.0.1", 0), handler) as server:
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            try:
                yield f"http://127.0.0.1:{server.server_address[1]}/index.html"
            finally:
                server.shutdown()
                thread.join(timeout=5)


def launch_fixture(playwright, executable: str, fixture: Path):
    profile = tempfile.TemporaryDirectory(prefix=f"venom-{fixture.name}-")
    context = playwright.chromium.launch_persistent_context(
        profile.name,
        executable_path=executable,
        headless=False,
        args=[
            f"--disable-extensions-except={fixture}",
            f"--load-extension={fixture}",
            "--no-first-run",
            "--disable-default-apps",
            "--disable-component-update",
            "--no-sandbox",
            "--disable-dev-shm-usage",
        ],
    )
    page = context.pages[0] if context.pages else context.new_page()
    page.goto("chrome://extensions/")
    extension_id = page.wait_for_function("""() => {
      const manager = document.querySelector('extensions-manager');
      const list = manager?.shadowRoot?.querySelector('extensions-item-list');
      const item = list?.shadowRoot?.querySelector('extensions-item');
      return item?.id || item?.getAttribute('id') || null;
    }""", timeout=12000).json_value()
    if not extension_id:
        context.close(); profile.cleanup()
        raise AssertionError(f"{fixture.name}: extension id was not discoverable")
    return context, profile, extension_id


def check_popup(playwright, executable: str, fixture: Path) -> None:
    context, profile, extension_id = launch_fixture(playwright, executable, fixture)
    try:
        page = context.new_page()
        manifest = json.loads(page.request.get(f"chrome-extension://{extension_id}/manifest.json").text())
        popup = manifest.get("action", {}).get("default_popup", "popup.html")
        page.goto(f"chrome-extension://{extension_id}/{popup}")
        page.wait_for_function("document.querySelector('#status')?.textContent === 'ready'")
    finally:
        context.close(); profile.cleanup()


def check_worker(playwright, executable: str, fixture: Path) -> None:
    context, profile, extension_id = launch_fixture(playwright, executable, fixture)
    try:
        page = context.new_page()
        page.goto(f"chrome-extension://{extension_id}/manifest.json")
        result = page.evaluate("chrome.runtime.sendMessage({type:'PING'})")
        assert result == {"ok": True, "kind": "service-worker"}, result
    finally:
        context.close(); profile.cleanup()


def check_content(playwright, executable: str, fixture: Path, marker: str, main_world: bool) -> None:
    context, profile, _extension_id = launch_fixture(playwright, executable, fixture)
    try:
        with local_page_server() as url:
            page = context.new_page()
            page.goto(url)
            page.wait_for_function(f"document.documentElement.dataset.{marker} === 'ready'")
            if main_world:
                assert page.evaluate("window.__venomMainWorld === true") is True
    finally:
        context.close(); profile.cleanup()


def check_offscreen(playwright, executable: str, fixture: Path) -> None:
    context, profile, extension_id = launch_fixture(playwright, executable, fixture)
    try:
        page = context.new_page()
        page.goto(f"chrome-extension://{extension_id}/manifest.json")
        first = page.evaluate("chrome.runtime.sendMessage({type:'PING'})")
        assert first == {"ok": True, "kind": "offscreen"}, first
        # A second call proves the already-created offscreen document remains usable.
        second = page.evaluate("chrome.runtime.sendMessage({type:'PING'})")
        assert second == first
    finally:
        context.close(); profile.cleanup()


def main() -> int:
    ensure_display()
    parser = argparse.ArgumentParser()
    parser.add_argument("--chromium")
    parser.add_argument("--fixtures", default=str(Path(__file__).resolve().parents[1] / "fixtures/chrome-extensions"))
    args = parser.parse_args()
    executable = chromium_path(args.chromium)
    fixtures = Path(args.fixtures).resolve()
    from playwright.sync_api import sync_playwright
    with sync_playwright() as playwright:
        checks = [
            ("popup-page", lambda: check_popup(playwright, executable, fixtures / "popup-page")),
            ("service-worker-module", lambda: check_worker(playwright, executable, fixtures / "service-worker-module")),
            ("content-isolated", lambda: check_content(playwright, executable, fixtures / "content-isolated", "venomIsolated", False)),
            ("content-main", lambda: check_content(playwright, executable, fixtures / "content-main", "venomMain", True)),
            ("offscreen-runtime", lambda: check_offscreen(playwright, executable, fixtures / "offscreen-runtime")),
        ]
        for name, check in checks:
            print(f"[RUN] {name}", flush=True)
            check()
            print(f"[PASS] {name}", flush=True)
    print("Chrome extension Chromium compatibility: PASS (5 runtime fixtures)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

"""Shared terminal output for Venom Python tools."""
from __future__ import annotations
import os, sys

_USE_COLOR = sys.stdout.isatty() and "NO_COLOR" not in os.environ
_CODES = {"INFO":"36", "PASS":"1;32", "SUCCESS":"1;32", "WARN":"1;33", "ERROR":"1;31", "FAIL":"1;31", "SERVER":"1;35"}

def status(label: str, message: str, *, stream=None) -> None:
    stream = stream or (sys.stderr if label in {"ERROR", "FAIL"} else sys.stdout)
    tag = f"[{label}]"
    if _USE_COLOR:
        tag = f"\x1b[{_CODES.get(label, '0')}m{tag}\x1b[0m"
    print(f"{tag:<10} {message}", file=stream, flush=True)

def info(message: str) -> None: status("INFO", message)
def passed(message: str) -> None: status("PASS", message)
def success(message: str) -> None: status("SUCCESS", message)
def warn(message: str) -> None: status("WARN", message)
def error(message: str) -> None: status("ERROR", message)
def server(message: str) -> None: status("SERVER", message)

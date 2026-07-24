#!/usr/bin/env python3
"""Reject source-machine paths and debug identities in a release WebAssembly artifact."""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

DEFAULT_FORBIDDEN = (
    b"C:\\Users\\",
    b"/Users/",
    b"/home/",
    b"\\Desktop\\",
    b"/Desktop/",
    b"turbojs_runtime_scaffold.c",
    b"venom-secure-web-runtime-v",
)


def printable_strings(data: bytes, minimum: int = 6) -> list[bytes]:
    return re.findall(rb"[ -~]{%d,}" % minimum, data)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("artifact", type=Path)
    parser.add_argument("--forbid", action="append", default=[])
    args = parser.parse_args()
    try:
        data = args.artifact.read_bytes()
    except OSError as exc:
        print(f"[venom] WASM release string check failed: {exc}", file=sys.stderr)
        return 2
    forbidden = list(DEFAULT_FORBIDDEN) + [value.encode("utf-8") for value in args.forbid]
    strings = printable_strings(data)
    violations: list[str] = []
    for token in forbidden:
        for value in strings:
            if token in value:
                sample = value[:180].decode("ascii", errors="replace")
                violations.append(f"{token!r} in {sample!r}")
                break
    # Catch absolute drive paths beyond C:\Users\ without rejecting benign ABI text.
    for value in strings:
        if re.search(rb"[A-Za-z]:\\[^\x00\r\n]{3,}", value):
            sample = value[:180].decode("ascii", errors="replace")
            violations.append(f"absolute Windows path in {sample!r}")
            break
    if violations:
        print("[venom] release WASM contains forbidden build identity strings:", file=sys.stderr)
        for violation in violations:
            print(f"  - {violation}", file=sys.stderr)
        return 2
    print(f"[venom] WASM release string check: PASS ({args.artifact}, {len(data)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Fail when a hardened Venom distribution exposes build/reverse-engineering metadata."""
from __future__ import annotations
import argparse
import re
import sys
from pathlib import Path

REPORT_NAMES = {
    "execution-plan.txt", "execution-plan.json", "function-plan.txt", "function-plan.json",
    "function-extraction-plan.txt", "function-extraction-plan.json",
    "runtime-bridge-contract.json", "bridge-rewrite-plan.json",
}
PUBLIC_METADATA = {
    "readme", "readme.md", "readme.txt", "changes", "changes.md", "changelog",
    "changelog.md", "contributing", "contributing.md", "security", "security.md",
}
TEXT_EXTENSIONS = {".js", ".json", ".html", ".css", ".txt", ".md", ".map"}
FORBIDDEN_TEXT = (
    re.compile(rb"(?:^|[/'\"\\])(?:js|src)/[A-Za-z0-9_.\-/]+\.js"),
    re.compile(rb"function-extraction-plan|runtime-bridge-contract|bridge-rewrite-plan"),
    re.compile(rb"VENOM_TJS_RUNTIME_ABI_V\d+"),
    re.compile(rb"venom_tjs_[A-Za-z0-9_]+"),
    re.compile(rb"bridge-result|bridge-error|type:[\'\"](?:invoke|cancel)[\'\"]"),
)

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("dist", type=Path)
    args = ap.parse_args()
    dist = args.dist.resolve()
    if not dist.is_dir():
        print(f"error: distribution directory does not exist: {dist}", file=sys.stderr)
        return 2
    problems: list[str] = []
    for path in sorted(p for p in dist.rglob("*") if p.is_file()):
        rel = path.relative_to(dist).as_posix()
        low = path.name.lower()
        if path.name in REPORT_NAMES or "/build/reports/" in f"/{rel}":
            problems.append(f"forbidden production report: {rel}")
        if "/" not in rel and low in PUBLIC_METADATA:
            problems.append(f"root project metadata copied into distribution: {rel}")
        # Public assets should not contain descriptive bridge/WASM ABI or original source paths.
        if path.suffix.lower() in TEXT_EXTENSIONS or path.suffix.lower() == ".wasm":
            data = path.read_bytes()
            for pattern in FORBIDDEN_TEXT:
                match = pattern.search(data)
                if match:
                    token = match.group(0)[:120].decode("utf-8", "replace")
                    problems.append(f"metadata leak in {rel}: {token}")
                    break
            if path.suffix.lower() == ".js":
                for marker in (b"VAEAD001", b"VSEAL001", b"VSODIUM1", b"deriveAeadStreamSeed", b"xorSealStream"):
                    if marker in data:
                        problems.append(f"browser package decoder leak in {rel}: {marker.decode('ascii')}")
                        break
    if problems:
        print("production leak scan: FAIL", file=sys.stderr)
        for problem in problems:
            print(f"  - {problem}", file=sys.stderr)
        return 1
    print("production leak scan: PASS")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

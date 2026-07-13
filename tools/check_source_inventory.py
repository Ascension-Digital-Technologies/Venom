#!/usr/bin/env python3
"""Reject accidentally orphaned first-party native source files.

WASM-only translation units are built by the Emscripten scripts rather than the
native CMake graph and are listed explicitly below. Everything else under src/
and fuzz/ must be named in CMakeLists.txt.
"""
from __future__ import annotations

import sys
from pathlib import Path

SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx"}
EXTERNAL_WASM_SOURCES = {
    "src/runtime/quickjs_runtime_scaffold.c",
    "src/runtime/wasm_runtime.c",
}
IGNORED_PREFIXES = ("third_party/",)


def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
    cmake_files = [root / "CMakeLists.txt", *(root / "cmake").glob("*.cmake")]
    cmake = "\n".join(path.read_text(encoding="utf-8") for path in cmake_files if path.is_file())
    missing: list[str] = []
    for base in (root / "src", root / "fuzz"):
        for path in base.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in SOURCE_SUFFIXES:
                continue
            rel = path.relative_to(root).as_posix()
            if rel in EXTERNAL_WASM_SOURCES or rel.startswith(IGNORED_PREFIXES):
                continue
            if rel not in cmake:
                missing.append(rel)

    for rel in sorted(EXTERNAL_WASM_SOURCES):
        if not (root / rel).is_file():
            missing.append(f"missing declared external WASM source: {rel}")

    if missing:
        print("source inventory check failed:", file=sys.stderr)
        for item in sorted(missing):
            print(f"  - {item}", file=sys.stderr)
        return 1
    print("source inventory check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

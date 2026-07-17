#!/usr/bin/env python3
"""Validate public build entry points without requiring platform-specific toolchains."""
from __future__ import annotations
import pathlib, re, sys


def require(path: pathlib.Path, needles: tuple[str, ...], failures: list[str]) -> None:
    if not path.is_file():
        failures.append(f"missing {path}")
        return
    text = path.read_text(encoding="utf-8", errors="replace")
    for needle in needles:
        if needle not in text:
            failures.append(f"{path}: missing contract marker {needle!r}")


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: build-scripts-smoke.py <repo-root>", file=sys.stderr)
        return 2
    root = pathlib.Path(sys.argv[1]).resolve()
    failures: list[str] = []
    require(root / "scripts" / "build-quickjs-wasm.sh", ("--preflight-only", "--verify-only", "build_emscripten.py"), failures)
    require(root / "scripts" / "build-quickjs-wasm.ps1", ("PreflightOnly", "VerifyOnly", "build_emscripten.py"), failures)
    require(root / "scripts" / "build-emscripten.sh", ("build_emscripten.py",), failures)
    require(root / "scripts" / "build-emscripten.ps1", ("build_emscripten.py",), failures)
    require(root / "tools" / "build_emscripten.py", ("--preflight-only", "--allow-missing", "--out-dir"), failures)
    cmake = root / "CMakeLists.txt"
    if not cmake.is_file() or "cmake/tests.cmake" not in cmake.read_text(encoding="utf-8"):
        failures.append("CMakeLists.txt does not include cmake/tests.cmake")
    for path in (root / "scripts").glob("*.sh"):
        first = path.read_text(encoding="utf-8", errors="replace").splitlines()[:1]
        if first and not first[0].startswith("#!"):
            failures.append(f"{path}: shell script has no shebang")
    if failures:
        for item in failures: print("failure:", item)
        return 1
    print("build scripts smoke: PASS")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

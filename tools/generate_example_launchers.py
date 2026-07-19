#!/usr/bin/env python3
"""Generate the thin platform launchers declared by contracts/examples.json."""
from __future__ import annotations

import argparse
import os
from pathlib import Path

from venom_tools.examples import load_example_registry
from venom_tools.io import atomic_write_text


def linux_wrapper(example: str) -> str:
    return f'''#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${{BASH_SOURCE[0]}}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
exec python3 "$ROOT/tools/launch_example.py" {example} "$@"
'''


def windows_wrapper(example: str) -> str:
    return fr'''@echo off
setlocal
set "VENOM_ROOT=%~dp0..\.."
set "VENOM_EXAMPLE={example}"
call "%VENOM_ROOT%\tools\windows-scripts\internal\invoke-example.bat" %*
set "VENOM_EXIT=%ERRORLEVEL%"
endlocal & exit /b %VENOM_EXIT%
'''


def expected_files(root: Path) -> dict[Path, str]:
    registry = load_example_registry(root)
    files: dict[Path, str] = {}
    for spec in registry.examples:
        base = f"build-and-launch-{spec.launcher}"
        files[root / "scripts" / "linux" / f"{base}.sh"] = linux_wrapper(spec.launcher)
        files[root / "scripts" / "windows" / f"{base}.bat"] = windows_wrapper(spec.launcher)
    return files


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--check", action="store_true", help="fail instead of rewriting drifted launchers")
    args = parser.parse_args()
    root = args.repo_root.resolve()
    expected = expected_files(root)
    failures: list[str] = []

    expected_paths = set(expected)
    for platform in ("linux", "windows"):
        suffix = ".sh" if platform == "linux" else ".bat"
        for path in (root / "scripts" / platform).glob(f"build-and-launch-*{suffix}"):
            if path.name == f"build-and-launch-website{suffix}":
                continue
            if path not in expected_paths:
                failures.append(f"undeclared example launcher: {path.relative_to(root)}")

    for path, content in expected.items():
        current = path.read_text(encoding="utf-8") if path.is_file() else None
        if current == content:
            continue
        if args.check:
            failures.append(f"launcher drift: {path.relative_to(root)}")
            continue
        atomic_write_text(path, content)
        if path.suffix == ".sh":
            os.chmod(path, 0o755)
        print(f"generated {path.relative_to(root)}")

    if failures:
        for failure in failures:
            print(failure)
        return 1
    if args.check:
        print(f"example launcher drift check passed ({len(expected)} launchers)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

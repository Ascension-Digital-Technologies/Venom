#!/usr/bin/env python3
"""Regenerate committed source snapshots and fail when authored inputs drift."""
from __future__ import annotations

import argparse
import difflib
import sys
import tempfile
from pathlib import Path

from venom_tools.process import run_command


def compare(expected: Path, actual: Path, root: Path) -> list[str]:
    expected_text = expected.read_text(encoding="utf-8")
    actual_text = actual.read_text(encoding="utf-8")
    if expected_text == actual_text:
        return []
    diff = "".join(difflib.unified_diff(
        actual_text.splitlines(keepends=True),
        expected_text.splitlines(keepends=True),
        fromfile=str(actual.relative_to(root)),
        tofile="regenerated",
        n=2,
    ))
    return [f"generated artifact drift: {actual.relative_to(root)}\n{diff[:6000]}"]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("repo_root", nargs="?", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    root = args.repo_root.resolve()
    failures: list[str] = []

    with tempfile.TemporaryDirectory(prefix="venom-generated-check-") as temporary:
        temp = Path(temporary)
        browser = temp / "browser_runtime.js"
        result = run_command([
            sys.executable,
            root / "tools/generators/runtime/bundle_js_modules.py",
            "--input-dir", root / "src/templates/browser",
            "--output", browser,
        ], cwd=root, timeout=60)
        if not result.passed:
            failures.append("browser runtime regeneration failed:\n" + result.output)
        else:
            failures.extend(compare(browser, root / "src/generated/runtime/javascript/browser_runtime.js", root))

        quickjs = temp / "quickjs_engine_module.cpp"
        result = run_command([
            sys.executable,
            root / "tools/generators/runtime/generate_quickjs_engine_module.py",
            "--input-dir", root / "src/templates/quickjs-engine",
            "--prefix", root / "tools/generators/runtime/templates/quickjs_engine_module.prefix.cpp",
            "--suffix", root / "tools/generators/runtime/templates/quickjs_engine_module.suffix.cpp",
            "--output", quickjs,
        ], cwd=root, timeout=60)
        if not result.passed:
            failures.append("QuickJS module regeneration failed:\n" + result.output)
        else:
            failures.extend(compare(quickjs, root / "src/generated/runtime/quickjs_engine_module.cpp", root))

    if failures:
        print("generated artifact check failed:")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    print("generated artifact check passed: browser runtime and QuickJS module snapshots are synchronized")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

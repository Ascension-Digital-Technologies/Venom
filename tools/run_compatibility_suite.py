#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path


def run(cmd: list[str], cwd: Path) -> None:
    print("[venom]", " ".join(cmd), flush=True)
    completed = subprocess.run(cmd, cwd=cwd)
    if completed.returncode:
        raise SystemExit(completed.returncode)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build and validate the canonical Venom compatibility fixture suite."
    )
    parser.add_argument("--venom", type=Path, required=True)
    parser.add_argument("--suite", type=Path, default=Path("tests/compatibility-suite.json"))
    parser.add_argument("--browser", default="all", choices=["all", "chromium", "firefox", "webkit"])
    parser.add_argument("--profile", default="prod", choices=["prod"])
    parser.add_argument("--out", type=Path, default=Path("build/compatibility"))
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--required-browsers", default="chromium,firefox,webkit")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    suite_path = (repo / args.suite).resolve() if not args.suite.is_absolute() else args.suite
    suite = json.loads(suite_path.read_text(encoding="utf-8"))
    if suite.get("schema_version") != 1 or not isinstance(suite.get("fixtures"), list):
        raise SystemExit("invalid compatibility suite manifest")

    venom = args.venom.resolve()
    if not venom.is_file():
        raise SystemExit(f"Venom executable not found: {venom}")

    output_root = (repo / args.out).resolve() if not args.out.is_absolute() else args.out.resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    reports: list[Path] = []

    for fixture in suite["fixtures"]:
        fixture_id = fixture["id"]
        site = (repo / fixture["site"]).resolve()
        dist = output_root / fixture_id
        report = output_root / f"{fixture_id}.browser.json"
        manifest = site / "venom.browser.json"
        if not site.is_dir() or not manifest.is_file():
            raise SystemExit(f"fixture is incomplete: {fixture_id}")
        if not args.skip_build:
            run([str(venom), "build", str(site), "--out", str(dist), "--profile", args.profile], repo)
        run([
            sys.executable,
            str(repo / "tools/browser_validation.py"),
            str(dist),
            "--browser", args.browser,
            "--manifest", str(manifest),
            "--json-out", str(report),
        ], repo)
        reports.append(report)

    matrix = output_root / "compatibility-matrix.json"
    markdown = output_root / "COMPATIBILITY.md"
    command = [
        sys.executable,
        str(repo / "tools/compatibility_matrix.py"),
        *[str(path) for path in reports],
        "--json-out", str(matrix),
        "--markdown-out", str(markdown),
        "--required-browsers", args.required_browsers,
    ]
    run(command, repo)
    print(f"[venom] compatibility matrix: {matrix}")
    print(f"[venom] compatibility report: {markdown}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

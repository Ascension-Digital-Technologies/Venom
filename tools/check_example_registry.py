#!/usr/bin/env python3
"""Validate the canonical example registry against repository contents."""
from __future__ import annotations

import argparse
import re
from pathlib import Path

from venom_tools.examples import load_example_registry


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("repo_root", nargs="?", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    root = args.repo_root.resolve()
    registry = load_example_registry(root)
    failures: list[str] = []

    declared_dirs = {spec.directory for spec in registry.examples}
    actual_dirs = {path.name for path in (root / "examples").iterdir() if path.is_dir()}
    missing = declared_dirs - actual_dirs
    undeclared = actual_dirs - declared_dirs
    if missing:
        failures.append("missing example directories: " + ", ".join(sorted(missing)))
    if undeclared:
        failures.append("undeclared example directories: " + ", ".join(sorted(undeclared)))

    for spec in registry.examples:
        site = root / spec.path
        if site.resolve().parent != (root / "examples").resolve():
            failures.append(f"{spec.id}: path must be a direct child of examples/")
        if not (site / "README.md").is_file():
            failures.append(f"{spec.id}: README.md is missing")
        if not (site / "venom.toml").is_file() and not (site / "venom.browser.json").is_file():
            failures.append(f"{spec.id}: venom.toml or venom.browser.json is missing")
        if not re.fullmatch(r"[a-z0-9]+(?:-[a-z0-9]+)*", spec.id):
            failures.append(f"{spec.id}: id must be kebab-case")
        if not re.fullmatch(r"[a-z0-9]+(?:-[a-z0-9]+)*", spec.launcher):
            failures.append(f"{spec.id}: launcher must be kebab-case")
        for platform, suffix in (("linux", ".sh"), ("windows", ".bat")):
            launcher = root / "scripts" / platform / f"build-and-launch-{spec.launcher}{suffix}"
            if not launcher.is_file():
                failures.append(f"{spec.id}: missing {launcher.relative_to(root)}")

    if failures:
        print("example registry check failed:")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    print(
        f"example registry check passed: {len(registry.examples)} examples, "
        f"{len(registry.certifiable())} certifiable, {len(registry.lookup())} launch names"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

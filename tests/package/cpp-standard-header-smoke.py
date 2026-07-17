#!/usr/bin/env python3
"""Catch direct standard-library include regressions in split compiler units."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[2]
checks = {
    root / "src/compiler/pipeline/js_rewriting.cpp": {
        "std::function": "#include <functional>",
    },
}

failures = []
for path, requirements in checks.items():
    text = path.read_text(encoding="utf-8")
    for symbol, header in requirements.items():
        if symbol in text and header not in text:
            failures.append(f"{path.relative_to(root)} uses {symbol} without {header}")

if failures:
    print("C++ standard header smoke: FAIL", file=sys.stderr)
    for failure in failures:
        print(f"- {failure}", file=sys.stderr)
    raise SystemExit(1)

print("C++ standard header smoke: PASS")

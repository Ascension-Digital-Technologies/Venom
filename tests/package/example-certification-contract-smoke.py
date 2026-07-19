#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / "tools"))
from venom_tools.examples import load_example_registry

registry = load_example_registry(root)
ids = {spec.id for spec in registry.examples}
actual = {path.name for path in (root / "examples").iterdir() if path.is_dir()}
assert ids == actual, (ids, actual)
assert registry.build_profile == "prod"
assert all(spec.requires_real_quickjs and spec.leak_scan for spec in registry.certifiable())
assert all(spec.browser for spec in registry.certifiable())
assert {"quickjs-benchmark", "chrome-extension"} <= {spec.id for spec in registry.examples if not spec.certify}
print("example certification contract smoke: PASS")

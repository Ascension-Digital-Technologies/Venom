#!/usr/bin/env python3
from __future__ import annotations
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

venom = Path(sys.argv[1]).resolve()
repo = Path(__file__).resolve().parents[2]
example = repo / "examples" / "quickjs-benchmark"
required = [
    "index.html",
    "browser/app.js",
    "protected/benchmark.js",
    "protected/index.js",
    "styles/main.css",
    "README.md",
    "venom.toml",
    "venom.lock",
]
for relative in required:
    if not (example / relative).is_file():
        raise SystemExit(f"QuickJS benchmark example is missing {relative}")

html = (example / "index.html").read_text(encoding="utf-8")
for marker in ('id="start-benchmark"', 'id="results-body"', 'src="./browser/app.js"'):
    if marker not in html:
        raise SystemExit(f"QuickJS benchmark HTML is missing {marker}")

benchmark = (example / "protected/benchmark.js").read_text(encoding="utf-8")
for marker in ("runBenchmarkCase", "benchmarkIdentity", "typedArrays", "mapsSets", "allocation"):
    if marker not in benchmark:
        raise SystemExit(f"QuickJS benchmark protected suite is missing {marker}")

with tempfile.TemporaryDirectory() as tmp:
    tmp_path = Path(tmp)
    out = tmp_path / "dist"
    cache = tmp_path / "cache"
    env = dict(os.environ)
    env["SOURCE_DATE_EPOCH"] = "1704067200"
    run = subprocess.run(
        [str(venom), "build", str(example), "--profile", "prod", "--out", str(out), "--cache-dir", str(cache), "--verbose"],
        cwd=repo,
        env=env,
        text=True,
        capture_output=True,
    )
    if run.returncode != 0:
        raise SystemExit(run.stdout + run.stderr)

    ir_path = cache / "project-ir.json"
    if not ir_path.is_file():
        raise SystemExit("QuickJS benchmark project IR was not emitted")
    ir = json.loads(ir_path.read_text(encoding="utf-8"))
    protected_sources = {item.get("source") for item in ir.get("protected_modules", [])}
    if "protected/benchmark.js" not in protected_sources:
        raise SystemExit("benchmark.js was not planned as protected code")

    metadata_files = list(out.rglob("metadata.json")) + list(out.rglob("manifest.json"))
    emitted_text = "\n".join(path.read_text(encoding="utf-8", errors="ignore") for path in metadata_files)
    if emitted_text:
        for exported in ("runBenchmarkCase", "benchmarkIdentity"):
            if exported not in emitted_text:
                raise SystemExit(f"QuickJS benchmark package metadata is missing {exported}")

    browser_javascript = "\n".join(path.read_text(encoding="utf-8", errors="ignore") for path in out.rglob("*.js"))
    for protected_marker in ("function arithmetic(", "function typedArrays(", "function allocation("):
        if protected_marker in browser_javascript:
            raise SystemExit(f"protected benchmark implementation leaked into browser JavaScript: {protected_marker}")

print("QuickJS benchmark example smoke: PASS")

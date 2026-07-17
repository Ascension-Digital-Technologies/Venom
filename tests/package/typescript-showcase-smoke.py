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
example = repo / "examples" / "typescript-showcase"
required = [
    "index.html",
    "browser/app.ts",
    "protected/types.ts",
    "protected/constants.ts",
    "protected/math.ts",
    "protected/pricing.ts",
    "protected/index.ts",
    "styles/main.css",
    "venom.toml",
    "venom.lock",
]
for relative in required:
    if not (example / relative).is_file():
        raise SystemExit(f"TypeScript showcase is missing {relative}")

html = (example / "index.html").read_text(encoding="utf-8")
for marker in ('type="application/json"', 'type="application/ld+json"', 'src="./browser/app.ts"'):
    if marker not in html:
        raise SystemExit(f"TypeScript showcase HTML is missing {marker}")

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

    combined = run.stdout + run.stderr
    if "typescript=" not in combined:
        raise SystemExit("TypeScript module count was not reported")

    ir_path = cache / "project-ir.json"
    if not ir_path.is_file():
        raise SystemExit("TypeScript showcase project IR was not emitted")
    ir = json.loads(ir_path.read_text(encoding="utf-8"))
    if ir.get("version") != 12:
        raise SystemExit(f"expected project IR v12, got {ir.get('version')}")

    modules = {module["source"]: module for module in ir.get("protected_modules", [])}
    expected_ts = {
        "protected/constants.ts",
        "protected/math.ts",
        "protected/pricing.ts",
    }
    missing = sorted(name for name in expected_ts if name not in modules)
    if missing:
        raise SystemExit(f"protected TypeScript modules missing from project IR: {missing}")
    for name in expected_ts:
        if not modules[name].get("typescript_transpiled"):
            raise SystemExit(f"{name} was not transpiled as TypeScript")

    declarations = list(out.rglob("venom-exports.d.ts"))
    if not declarations:
        raise SystemExit("typed protected API declaration was not generated")
    declaration_text = declarations[0].read_text(encoding="utf-8")
    for exported in ("calculateQuote", "runtimeIdentity"):
        if exported not in declaration_text:
            raise SystemExit(f"generated protected API is missing {exported}")

    if not list(out.rglob("*.vqc")):
        raise SystemExit("compiled route payload was not emitted")

    source_extensions = {".ts", ".tsx", ".mts", ".cts", ".jsx"}
    leaked_sources = [path.relative_to(out).as_posix() for path in out.rglob("*") if path.is_file() and path.suffix.lower() in source_extensions and not path.name.endswith(".d.ts")]
    if leaked_sources:
        raise SystemExit(f"TypeScript or JSX source files leaked into the distribution: {leaked_sources}")

    browser_javascript = "\n".join(
        path.read_text(encoding="utf-8", errors="ignore")
        for path in out.rglob("*.js")
    )
    for protected_marker in ("calculateRisk", "couponRate", "maximumDiscount"):
        if protected_marker in browser_javascript:
            raise SystemExit(f"protected TypeScript implementation leaked into browser JavaScript: {protected_marker}")

print("TypeScript showcase smoke: PASS")

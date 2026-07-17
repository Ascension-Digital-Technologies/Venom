#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path

venom = Path(sys.argv[1]).resolve()
repo = Path(__file__).resolve().parents[2]
example = repo / "examples" / "tsx-showcase"

with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    dist = root / "dist"
    cache = root / "cache"
    env = dict(os.environ)
    env["SOURCE_DATE_EPOCH"] = "1704067200"

    build = subprocess.run(
        [str(venom), "build", str(example), "--profile", "prod", "--out", str(dist), "--cache-dir", str(cache), "--quiet"],
        cwd=repo,
        env=env,
        text=True,
        capture_output=True,
    )
    if build.returncode != 0:
        raise SystemExit(build.stdout + build.stderr)

    verify = subprocess.run(
        [str(venom), "verify", str(dist), "--target", "browser", "--format", "json"],
        cwd=repo,
        env=env,
        text=True,
        capture_output=True,
    )
    if verify.returncode != 0:
        raise SystemExit(verify.stdout + verify.stderr)
    if '"ok":true' not in verify.stdout.replace(" ", "").lower():
        raise SystemExit(f"production verification did not return ok=true:\n{verify.stdout}")

    runtime = subprocess.run(
        [str(venom), "verify-runtime", str(dist), "--require-real-engine", "--format", "json"],
        cwd=repo,
        env=env,
        text=True,
        capture_output=True,
    )
    if runtime.returncode != 0:
        raise SystemExit(runtime.stdout + runtime.stderr)
    if '"ok":true' not in runtime.stdout.replace(" ", "").lower():
        raise SystemExit(f"runtime verification did not return ok=true:\n{runtime.stdout}")

print("TSX showcase production verification smoke: PASS")

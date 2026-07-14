#!/usr/bin/env python3
from __future__ import annotations
import json
import subprocess
import sys
import tempfile
from pathlib import Path

repo = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as td:
    root = Path(td)
    reports = []
    for browser in ("chromium", "firefox", "webkit"):
        report = root / f"{browser}.json"
        report.write_text(json.dumps({
            "fixture_id": "synthetic-framework-output",
            "fixture_profile": "framework-production-output",
            "evidence_level": "behavioral",
            "support_tier": "supported",
            "framework": {"name": "Synthetic", "version": "1"},
            "claims": ["hydration", "events"],
            "results": [{"browser": browser, "passed": True, "checks": [{}, {}, {}]}],
        }), encoding="utf-8")
        reports.append(report)
    matrix = root / "matrix.json"
    markdown = root / "COMPATIBILITY.md"
    command = [sys.executable, str(repo / "tools/compatibility_matrix.py"), *map(str, reports),
               "--json-out", str(matrix), "--markdown-out", str(markdown)]
    completed = subprocess.run(command, cwd=repo, capture_output=True, text=True)
    if completed.returncode:
        raise SystemExit(completed.stdout + completed.stderr)
    data = json.loads(matrix.read_text(encoding="utf-8"))
    assert data["schema_version"] == 3
    assert data["passed"] is True
    assert data["summary"]["supported_count"] == 1
    text = markdown.read_text(encoding="utf-8")
    assert "# Venom Compatibility Matrix" in text
    assert "synthetic-framework-output" in text
    assert "PASS (3)" in text
print("compatibility report smoke: PASS")

#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]
scripts = root / "scripts"
helper = (scripts / "internal" / "invoke-powershell.bat").read_text(encoding="utf-8").lower()
required = ["command completed successfully", "command failed with exit code", "press any key to close", "venom_no_pause", "github_actions"]
for marker in required:
    if marker not in helper:
        raise SystemExit(f"launcher policy missing marker: {marker}")
for bat in sorted(scripts.glob("*.bat")):
    ps1 = bat.with_suffix(".ps1")
    if not ps1.exists():
        continue
    text = bat.read_text(encoding="utf-8", errors="replace").lower()
    if "internal\\invoke-powershell.bat" not in text:
        raise SystemExit(f"Windows launcher bypasses shared policy: {bat.name}")
    if "pause" in text:
        raise SystemExit(f"Windows launcher contains duplicated pause logic: {bat.name}")

console = (scripts / "internal" / "console.ps1").read_text(encoding="utf-8")
for marker in ("Write-VenomBanner", "Write-VenomStep", "Write-VenomSuccess", "Write-VenomFailure"):
    if marker not in console:
        raise SystemExit(f"production console helper missing marker: {marker}")
for name in ("build.ps1", "build-site.ps1", "build-and-serve-example.ps1", "test.ps1", "release-closure.ps1"):
    text = (scripts / name).read_text(encoding="utf-8")
    if "internal/console.ps1" not in text:
        raise SystemExit(f"production workflow does not import console helper: {name}")

print("windows launcher policy smoke: PASS")

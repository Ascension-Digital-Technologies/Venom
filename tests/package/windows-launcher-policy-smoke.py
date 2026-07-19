#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]
scripts = root / "scripts" / "windows"
internal = root / "tools" / "windows-scripts" / "internal"

finish = (internal / "finish-command.bat").read_text(encoding="utf-8").lower()
for marker in (
    "command completed successfully",
    "command failed with exit code",
    "press any key to close",
    "venom_no_pause",
    "github_actions",
):
    if marker not in finish:
        raise SystemExit(f"launcher completion policy missing marker: {marker}")

for helper_name in ("invoke-powershell.bat", "invoke-example.bat"):
    helper = (internal / helper_name).read_text(encoding="utf-8").lower()
    if "finish-command.bat" not in helper:
        raise SystemExit(f"Windows helper bypasses shared completion policy: {helper_name}")
    if "pause" in helper:
        raise SystemExit(f"Windows helper duplicates pause policy: {helper_name}")

for bat in sorted(scripts.glob("build-and-launch-*.bat")):
    text = bat.read_text(encoding="utf-8", errors="replace").lower()
    if bat.name == "build-and-launch-website.bat":
        required = r"tools\windows-scripts\internal\finish-command.bat"
    else:
        required = r"tools\windows-scripts\internal\invoke-example.bat"
    if "\t" in text:
        raise SystemExit(f"Windows launcher contains a tab-corrupted path: {bat.name}")
    if required not in text:
        raise SystemExit(f"Windows launcher bypasses shared policy: {bat.name}")
    if "pause" in text:
        raise SystemExit(f"Windows launcher contains duplicated pause logic: {bat.name}")

for name in ("build-emsdk.bat", "build.bat", "package-release.bat", "verify-release.bat", "release-closure.bat"):
    text = (scripts / name).read_text(encoding="utf-8", errors="replace").lower()
    if "invoke-powershell.bat" not in text:
        raise SystemExit(f"PowerShell launcher bypasses shared policy: {name}")
    if "pause" in text:
        raise SystemExit(f"PowerShell launcher contains duplicated pause logic: {name}")

for name in ("package-chrome-extension.bat", "verify-chrome-extension-compatibility.bat"):
    text = (scripts / name).read_text(encoding="utf-8", errors="replace").lower()
    if "finish-command.bat" not in text:
        raise SystemExit(f"specialized launcher bypasses shared completion policy: {name}")
    if "pause" in text:
        raise SystemExit(f"specialized launcher contains duplicated pause logic: {name}")

console = (internal / "console.ps1").read_text(encoding="utf-8")
for marker in ("Write-VenomBanner", "Write-VenomStep", "Write-VenomSuccess", "Write-VenomFailure"):
    if marker not in console:
        raise SystemExit(f"production console helper missing marker: {marker}")

build_ps1 = (root / "tools" / "windows-scripts" / "build.ps1").read_text(encoding="utf-8")
if "internal/console.ps1" not in build_ps1:
    raise SystemExit("native build workflow does not import the shared console helper")

print("windows launcher policy smoke: PASS")

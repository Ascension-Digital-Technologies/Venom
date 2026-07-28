from __future__ import annotations
from _private_reports import root_for

import subprocess
import sys
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[2]
venom = Path(sys.argv[1])

with tempfile.TemporaryDirectory(prefix="venom-clean-assets-") as temp:
    dist = Path(temp) / "dist"
    subprocess.run(
        [
            str(venom),
            "build",
            str(root / "examples/protected-chess"),
            "--out",
            str(dist),
            "--profile",
            "prod",
            "--hashed",
        ],
        check=True,
        stdout=subprocess.DEVNULL,
    )

    assets = dist / "assets"
    assert assets.is_dir()
    assert not (assets / "venom.browser.json").exists()
    assert not any(path.name == "venom.browser.json" for path in dist.rglob("*"))

    root_files = sorted(path.name for path in assets.iterdir() if path.is_file())
    assert not any(name.lower().endswith(".txt") for name in root_files), root_files
    assert not any(name.lower().endswith(".json") for name in root_files), root_files

    assert not any(path.name.lower() in {"third-party-notices.txt", "third_party_notices.txt"} for path in dist.rglob("*"))
    assert not any(path.name == "build.json" for path in dist.rglob("*"))
    assert (root_for(dist) / "build.json").is_file()
    assert (root_for(dist) / "reports" / "bridge-rewrite-plan.json").is_file()

print("clean production asset layout smoke: PASS")

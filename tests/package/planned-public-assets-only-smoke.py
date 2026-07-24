#!/usr/bin/env python3
"""Prove production output is derived from the reachable public graph only."""

from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path


venom = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix="venom-planned-assets-") as temporary:
    root = Path(temporary)
    site = root / "site"
    # Exercise the normal in-project output layout. A stale dist must be pruned
    # from discovery before the new build graph is created.
    dist = site / "dist"
    (site / "assets" / "pieces").mkdir(parents=True)
    (site / "private").mkdir()

    (site / "index.html").write_text(
        "<!doctype html><link rel='stylesheet' href='app.css'>"
        "<img src='assets/logo.svg'><script src='app.js'></script>",
        encoding="utf-8",
    )
    (site / "app.css").write_text(
        "body{background:url('assets/background.png')}", encoding="utf-8"
    )
    (site / "app.js").write_text(
        "globalThis.piecePath='assets/pieces/{piece}.png';", encoding="utf-8"
    )
    (site / "assets" / "logo.svg").write_text("<svg/>", encoding="utf-8")
    (site / "assets" / "background.png").write_bytes(b"planned-background")
    (site / "assets" / "pieces" / "white.png").write_bytes(b"planned-piece")

    decoys = {
        "private/customer-list.csv": b"VENOM_PRIVATE_CUSTOMERS",
        "private/deploy.env": b"VENOM_PRIVATE_TOKEN=do-not-ship",
        "notes.txt": b"VENOM_PRIVATE_NOTES",
        "app.css.bak": b"VENOM_PRIVATE_CSS_BACKUP",
        "unlinked.css": b"/* VENOM_PRIVATE_UNLINKED_CSS */",
        "unlinked.js": b"globalThis.VENOM_PRIVATE_UNLINKED_JS=true;",
        "assets/unreferenced.png": b"VENOM_PRIVATE_UNREFERENCED_IMAGE",
    }
    for relative, payload in decoys.items():
        path = site / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(payload)

    (dist / "assets").mkdir(parents=True)
    (dist / "index.html").write_text(
        "<!doctype html><p>VENOM_PRIVATE_STALE_DISTRIBUTION</p>", encoding="utf-8"
    )
    (dist / "assets" / "stale.txt").write_text(
        "VENOM_PRIVATE_STALE_ASSET", encoding="utf-8"
    )
    (site / ".venom" / "dist").mkdir(parents=True)
    (site / ".venom" / "dist" / "build.json").write_text(
        "VENOM_PRIVATE_BUILD_REPORT", encoding="utf-8"
    )

    build = subprocess.run(
        [str(venom), "build", str(site), "--out", str(dist), "--profile", "prod", "--hashed"],
        check=True,
        text=True,
        capture_output=True,
    )
    assert "across 1 HTML routes" in build.stdout, build.stdout

    emitted = [path for path in dist.rglob("*") if path.is_file()]
    blob = b"\n".join(path.read_bytes() for path in emitted)
    for relative, marker in decoys.items():
        assert marker not in blob, f"unplanned input leaked into dist: {relative}"
    for marker in (
        b"VENOM_PRIVATE_STALE_DISTRIBUTION",
        b"VENOM_PRIVATE_STALE_ASSET",
        b"VENOM_PRIVATE_BUILD_REPORT",
    ):
        assert marker not in blob, f"private build material leaked into dist: {marker!r}"

    public_blob = b"\n".join(path.read_bytes() for path in emitted if path.suffix in {".png", ".svg"})
    for marker in (b"<svg/>", b"planned-background", b"planned-piece"):
        assert marker in public_blob, f"planned public asset was not emitted: {marker!r}"

    dangerous = root / "dangerous-output"
    dangerous.mkdir()
    sentinel = dangerous / "index.html"
    sentinel.write_text("source-must-survive", encoding="utf-8")
    rejected = subprocess.run(
        [str(venom), "build", str(dangerous), "--out", str(dangerous), "--profile", "prod"],
        text=True,
        capture_output=True,
    )
    assert rejected.returncode != 0, "build accepted an output directory equal to its input"
    assert sentinel.read_text(encoding="utf-8") == "source-must-survive", "rejected build damaged its input"

print("planned public assets only smoke: PASS")

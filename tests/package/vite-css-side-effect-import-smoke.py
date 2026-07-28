#!/usr/bin/env python3
import pathlib
import subprocess
import sys
import tempfile

root = pathlib.Path(__file__).resolve().parents[2]
venom = pathlib.Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else root / "build" / "release" / "venom"
site = root / "examples" / "react-vite-showcase"

with tempfile.TemporaryDirectory(prefix="venom-vite-css-") as temp:
    output = pathlib.Path(temp) / "dist"
    subprocess.run([str(venom), "build", str(site), "--out", str(output), "--profile", "prod", "--quiet"], check=True)
    styles = list((output / "assets" / "app").glob("*.css"))
    if len(styles) != 1:
        raise SystemExit(f"expected one generated stylesheet, found {len(styles)}")
    css = styles[0].read_text(encoding="utf-8")
    if ":root" not in css or "font-family: Inter" not in css:
        raise SystemExit("Vite side-effect CSS import was not bundled")

print("vite css side-effect import smoke: PASS")

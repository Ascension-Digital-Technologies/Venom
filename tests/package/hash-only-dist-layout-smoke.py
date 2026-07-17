#!/usr/bin/env python3
from pathlib import Path
import re, sys

root = Path(sys.argv[1])
expected_dirs = {"app", "javascript", "wasm", "images"}
assets = root / "assets"
if not (root / "index.html").is_file(): raise SystemExit("missing dist/index.html")
actual_dirs = {p.name for p in assets.iterdir() if p.is_dir()}
if actual_dirs != expected_dirs: raise SystemExit(f"unexpected assets directories: {sorted(actual_dirs)}")
if list(root.rglob("*.json")): raise SystemExit("production dist must not contain JSON files")
for f in assets.rglob("*"):
    if f.is_file() and f.name.startswith("."):
        raise SystemExit(f"dot-prefixed production asset is forbidden: {f.relative_to(root)}")
hex_name = re.compile(r"^[0-9a-f]{12,16}$")
for folder, exts in {"app": {".vbc", ".vqc", ".css"}, "javascript": {".js"}, "wasm": {".wasm"}}.items():
    for f in (assets / folder).iterdir():
        if not f.is_file(): continue
        if f.suffix not in exts or not hex_name.fullmatch(f.stem):
            raise SystemExit(f"noncanonical generated asset: {f.relative_to(root)}")
for f in (assets / "images").iterdir():
    if not f.is_file(): continue
    parts = f.name.split('.')
    if len(parts) < 3 or not hex_name.fullmatch(parts[-2]):
        raise SystemExit(f"noncanonical image asset: {f.relative_to(root)}")
text = (root / "index.html").read_text(encoding="utf-8")
if not re.search(r"assets/app/[0-9a-f]{16}\.css", text): raise SystemExit("index missing canonical CSS reference")
if not re.search(r"assets/javascript/[0-9a-f]{16}\.js", text): raise SystemExit("index missing canonical JS reference")
for forbidden in ("assets/loader/", "assets/runtime/", "assets/workers/", "assets/style/", "build.json", "loader.", "runtime.", "worker.", "registry-"):
    if forbidden in text: raise SystemExit(f"legacy label remains in index: {forbidden}")
print("hash-only production layout: PASS")

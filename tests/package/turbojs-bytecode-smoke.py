#!/usr/bin/env python3
import pathlib
from pathlib import Path


def find_package(dist: Path) -> Path:
    candidates = sorted((dist / 'assets' / 'app').glob('app.*.vbc'))
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise SystemExit('missing app package under assets/app/app.<hash>.vbc')
import subprocess
import sys

if len(sys.argv) != 4:
    print("usage: turbojs-bytecode-smoke.py <venom> <site> <out>")
    raise SystemExit(2)

venom = pathlib.Path(sys.argv[1])
site = pathlib.Path(sys.argv[2])
out = pathlib.Path(sys.argv[3])

proc = subprocess.run([str(venom), "build", str(site), "--out", str(out), "--profile", "prod"], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
if proc.returncode != 0:
    print(proc.stdout)
    raise SystemExit(proc.returncode)
if "turbojs_bytecode_records:" not in proc.stdout:
    print(proc.stdout)
    raise SystemExit("build protection report did not include TurboJS bytecode records")
if "turbojs_bytecode_records: 0" in proc.stdout:
    print(proc.stdout)
    raise SystemExit("build produced zero TurboJS bytecode records")

package = find_package(out)
blob = package.read_bytes()
for forbidden in (b"console.log", b"basic site script loaded", b"VTJSBC01", b"source-preserving-byte-buffer-record"):
    if forbidden in blob:
        raise SystemExit(f"protected package leaked forbidden TurboJS/source marker: {forbidden!r}")
for marker in (b"VTJSBC02", b"VTJSBC03"):
    if marker in blob:
        raise SystemExit(f"protected raw package exposes TurboJS bytecode marker; section should be sealed: {marker!r}")

check = subprocess.run([str(venom), "verify", str(out), "--target", "browser"], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
if check.returncode != 0:
    print(check.stdout)
    raise SystemExit(check.returncode)
if "release_status: PASS" not in check.stdout or "turbojs_bytecode_records: 0" in check.stdout:
    print(check.stdout)
    raise SystemExit("verify did not verify TurboJS bytecode records")

print("turbojs bytecode smoke passed")

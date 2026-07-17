#!/usr/bin/env python3
import os
import subprocess
import sys
from pathlib import Path


def find_package(dist: Path) -> Path:
    candidates = sorted((dist / 'assets' / 'app').glob('app.*.vbc'))
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise SystemExit('missing app package under assets/app/app.<hash>.vbc')

if len(sys.argv) < 4:
    print("usage: libsodium-provider-smoke.py <venom> <site> <out> [runtime_probe]")
    sys.exit(2)

venom = Path(sys.argv[1])
site = Path(sys.argv[2])
out = Path(sys.argv[3])
runtime_probe = Path(sys.argv[4]) if len(sys.argv) > 4 else None
key = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"
env = os.environ.copy()
env["VENOM_PACKAGE_KEY"] = key

cmd = [str(venom), "build", str(site), "--out", str(out), "--profile", "prod", "--crypto-provider", "libsodium"]
proc = subprocess.run(cmd, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
if proc.returncode != 0:
    if "libsodium provider is not available" in proc.stdout and env.get("VENOM_REQUIRE_LIBSODIUM_TEST") != "1":
        print("libsodium provider unavailable; skipping optional audited crypto smoke")
        print(proc.stdout)
        sys.exit(0)
    print(proc.stdout)
    sys.exit(proc.returncode)

package = find_package(out)
blob = package.read_bytes()
checks = {
    b"VSODIUM1": True,
    b"VAEAD001": False,
    b"VSEAL001": False,
    b"console.log": False,
    b"basic site script loaded": False,
    b"route-bytecode.vmbc": False,
}
for needle, should_exist in checks.items():
    found = needle in blob
    if found != should_exist:
        raise SystemExit(f"unexpected raw package marker {needle!r}: found={found}, expected={should_exist}")

inspect = subprocess.run([str(venom), "inspect", str(package)], env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
if inspect.returncode != 0:
    print(inspect.stdout)
    raise SystemExit(inspect.returncode)
if "sections:" not in inspect.stdout or "encrypted" not in inspect.stdout:
    print(inspect.stdout)
    raise SystemExit("inspect did not decode libsodium-sealed sections")

if runtime_probe and runtime_probe.exists():
    probe = subprocess.run([str(runtime_probe), str(package), "/"], env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if probe.returncode != 0:
        print(probe.stdout)
        raise SystemExit(probe.returncode)
    if "routes:" not in probe.stdout or "text:" not in probe.stdout:
        print(probe.stdout)
        raise SystemExit("native runtime probe did not decode libsodium package")

print("libsodium provider smoke passed")

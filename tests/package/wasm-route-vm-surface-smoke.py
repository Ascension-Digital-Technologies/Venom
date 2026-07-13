#!/usr/bin/env python3
from __future__ import annotations
import pathlib, shutil, subprocess, sys, tempfile

FORBIDDEN = (
    "function decodeVmInstruction",
    "function executeRoute",
    "function parseVmBytecode",
    "function parsePolymorphicMap",
    "const LOGICAL =",
    "route-bytecode.vmbc",
)
REQUIRED = (
    "VDOP0020",
    "routeVmOwner",
    "v12_x",
    "data-venom-wasm-runtime",
)

def main() -> int:
    if len(sys.argv) != 3:
        print("usage: wasm-route-vm-surface-smoke.py <venom> <repo>", file=sys.stderr)
        return 2
    venom = pathlib.Path(sys.argv[1]).resolve()
    repo = pathlib.Path(sys.argv[2]).resolve()
    with tempfile.TemporaryDirectory(prefix="venom-wasm-route-vm-") as td:
        dist = pathlib.Path(td) / "dist"
        subprocess.run([str(venom), "build", str(repo / "examples" / "no-script-site"), "--out", str(dist)], check=True)
        runtimes = list((dist / "assets" / "runtime").glob("r.*.js"))
        if len(runtimes) != 1:
            raise SystemExit(f"expected one runtime JS asset, found {len(runtimes)}")
        text = runtimes[0].read_text(encoding="utf-8")
        for marker in FORBIDDEN:
            if marker in text:
                raise SystemExit(f"protected runtime retained readable Route VM surface: {marker}")
        for marker in REQUIRED:
            if marker not in text:
                raise SystemExit(f"protected runtime missing WASM Route VM integration marker: {marker}")
        wasm = list((dist / "assets" / "runtime").glob("runtime.*.wasm"))
        if len(wasm) != 1 or wasm[0].stat().st_size < 1024:
            raise SystemExit("protected distribution is missing the Route VM WASM runtime")
    print("WASM Route VM protected-surface smoke: PASS")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

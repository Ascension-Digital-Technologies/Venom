#!/usr/bin/env python3
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile


def run(cmd):
    subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)


def runtime_js(dist: pathlib.Path) -> pathlib.Path:
    matches = list((dist / "assets" / "runtime").glob("r.*.js"))
    if len(matches) != 1:
        raise SystemExit(f"expected one protected runtime JS asset, found {len(matches)}")
    return matches[0]


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: runtime-surface-diversification-smoke.py <venom>", file=sys.stderr)
        return 2
    venom = pathlib.Path(sys.argv[1]).resolve()
    repo = pathlib.Path(__file__).resolve().parents[2]
    site = repo / "tests" / "fixtures" / "sites" / "no-script-site"
    with tempfile.TemporaryDirectory(prefix="venom-runtime-surface-") as td:
        root = pathlib.Path(td)
        a, b = root / "a", root / "b"
        run([str(venom), "build", str(site), "--out", str(a), "--no-cache"])
        run([str(venom), "build", str(site), "--out", str(b), "--no-cache"])
        ja = runtime_js(a).read_text(encoding="utf-8")
        jb = runtime_js(b).read_text(encoding="utf-8")
        if ja == jb:
            raise SystemExit("independent protected builds produced identical runtime JavaScript")
        forbidden = [
            "prepareVenomWasmRuntime", "installVenomWasmPackageParser",
            "installVenomHostBridge", "decodeWasmDomOpStream",
            "applyWasmDomOperations", "WASM runtime execution failed",
            "WASM-owned package parse failed", "invalid DOM handle authentication tag",
        ]
        for name in forbidden:
            if name in ja or name in jb:
                raise SystemExit(f"protected runtime retained semantic marker: {name}")
        opaque_pattern = r"\b(?:v[0-9a-f]{8}|_0x[0-9a-f]{4,})\b"
        opaque_a = set(re.findall(opaque_pattern, ja))
        opaque_b = set(re.findall(opaque_pattern, jb))
        if len(opaque_a) < 12 or len(opaque_b) < 12:
            raise SystemExit("protected runtime did not contain the expected opaque identifier set")
        if opaque_a == opaque_b:
            raise SystemExit("independent builds reused the same opaque identifier map")
        if not re.search(r"V-[0-9A-F]{6}", ja) or not re.search(r"V-[0-9A-F]{6}", jb):
            raise SystemExit("protected runtime did not contain compact build-specific error codes")
    print("runtime surface diversification smoke: PASS")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

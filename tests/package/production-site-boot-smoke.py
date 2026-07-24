#!/usr/bin/env python3
"""Build and boot a large production site with local and remote scripts."""

from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path


def run(command: list[str], cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(command, cwd=cwd, text=True, capture_output=True)
    if result.returncode != 0:
        raise RuntimeError(
            f"command failed ({result.returncode}): {' '.join(command)}\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
        )
    return result


def main() -> int:
    if len(sys.argv) != 4:
        print("usage: production-site-boot-smoke.py <venom> <output-root> <node>", file=sys.stderr)
        return 2

    venom = Path(sys.argv[1]).resolve()
    output_root = Path(sys.argv[2]).resolve()
    node = Path(sys.argv[3]).resolve()
    source_root = Path(__file__).resolve().parents[2]
    harness = source_root / "tests" / "runtime" / "browser-compat-harness.mjs"
    site = output_root / "site"
    dist = output_root / "dist"

    shutil.rmtree(output_root, ignore_errors=True)
    site.mkdir(parents=True)
    large_dom = "capacity-boundary-" * 3200
    (site / "index.html").write_text(
        "<!doctype html><html><head><meta charset=\"utf-8\"><title>Production boot</title></head>"
        "<body><main id=\"app\">" + large_dom + "</main>"
        "<script src=\"./fpCollect.min.js\"></script>"
        "<script src=\"./large-protected.js\"></script>"
        "<script src=\"https://cdn.example.invalid/vendor.js\"></script>"
        "<script>globalThis.__venomProductionBoot = typeof fpCollect === 'function' && globalThis.__venomLargeProtected === 1;</script>"
        "</body></html>",
        encoding="utf-8",
    )
    (site / "fpCollect.min.js").write_text(
        "const fpCollect = () => ({ ok: true }); globalThis.__venomFpCollectLoaded = true;\n",
        encoding="utf-8",
    )
    # Cross the old 256 KiB TurboJS transfer ceiling while remaining below the
    # production 768 KiB limit. This reproduces the real vendored Yandex bundle
    # failure without depending on the network.
    large_script = "globalThis.__venomLargeProtected = 1;\n/*" + ("x" * 540000) + "*/\n"
    (site / "large-protected.js").write_text(large_script, encoding="utf-8")
    (site / "venom.lock").write_text(
        "VENOM_VENDOR_LOCK_V1\n"
        "version=1\n"
        "entry_count=1\n"
        "url\tsha256\tbytes\tintegrity\n"
        "https://cdn.example.invalid/vendor.js\t8955b858b9126474dc62bb53e62a8a4038021253626557335ce2d08c72d401e9\t43\tnot-declared\n",
        encoding="utf-8",
    )

    remote_cache = source_root / "tests" / "fixtures" / "remote-cache"
    run([str(venom), "build", str(site), "--out", str(dist), "--vendor-cache", str(remote_cache), "--offline"])
    run([str(venom), "verify-runtime", str(dist), "--target", "browser", "--require-real-engine"])
    boot = run([
        str(node), str(harness), str(dist), "strict-no-eval",
        "--strict-no-source-eval", "--via-loader",
    ])
    combined = boot.stdout + boot.stderr
    if "Identifier 'fpCollect' has already been declared" in combined:
        raise RuntimeError("fpCollect declaration collision returned")
    if "WASM runtime execution failed: 14" in combined:
        raise RuntimeError("large DOM operation stream exceeded companion WASM capacity")
    if "browser compatibility harness passed: strict-no-eval:via-loader" not in combined:
        raise RuntimeError(f"production loader boot did not complete:\n{combined}")

    oversized_site = output_root / "oversized-site"
    oversized_dist = output_root / "oversized-dist"
    oversized_site.mkdir(parents=True)
    (oversized_site / "index.html").write_text(
        "<!doctype html><html><body><script src=\"./too-large.js\"></script></body></html>",
        encoding="utf-8",
    )
    (oversized_site / "too-large.js").write_text(
        "globalThis.__tooLarge = 1;\n/*" + ("y" * 790000) + "*/\n",
        encoding="utf-8",
    )
    rejected = subprocess.run(
        [str(venom), "build", str(oversized_site), "--out", str(oversized_dist)],
        text=True,
        capture_output=True,
    )
    rejected_text = rejected.stdout + rejected.stderr
    if rejected.returncode == 0 or "TurboJS script exceeds protected transfer limit" not in rejected_text:
        raise RuntimeError(f"oversized protected script was not rejected at build time:\n{rejected_text}")
    if oversized_dist.exists():
        raise RuntimeError("oversized script rejection left an incomplete production dist")

    print("production site boot smoke passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

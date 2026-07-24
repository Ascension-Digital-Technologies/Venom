#!/usr/bin/env python3
"""Build a production dist and boot it through the emitted loader.

This catches path regressions that direct runtime harnesses can miss: grouped
app/style/runtime/worker URLs, worker preflight, binding-token delivery, and
fail-closed no-source-eval boot through the public index/loader path.
"""
from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path
from _turbojs_artifact import require_current_artifact


def main() -> int:
    if len(sys.argv) != 5:
        print('usage: production-loader-smoke.py <venom> <site> <out-root> <node>', file=sys.stderr)
        return 2
    source_root = Path(__file__).resolve().parents[2]
    require_current_artifact(source_root)
    venom = Path(sys.argv[1]).resolve()
    site = Path(sys.argv[2]).resolve()
    out_root = Path(sys.argv[3]).resolve()
    node = sys.argv[4]
    harness = Path(__file__).resolve().parents[1] / 'runtime' / 'browser-compat-harness.mjs'

    if out_root.exists():
        shutil.rmtree(out_root)
    dist = out_root / 'dist'
    dist.parent.mkdir(parents=True, exist_ok=True)

    subprocess.run([str(venom), 'build', str(site), '--out', str(dist)], check=True)
    subprocess.run([str(venom), 'verify-runtime', str(dist), '--target', 'browser', '--require-real-engine'], check=True)
    subprocess.run([node, str(harness), str(dist), 'real-engine-strict', '--strict-no-source-eval', '--via-loader'], check=True)
    print('production loader smoke passed')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

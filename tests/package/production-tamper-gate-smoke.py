#!/usr/bin/env python3
"""Prove the production artifact chain fails closed after dist tampering."""
from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path
from _quickjs_artifact import require_current_artifact


def run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def expect_failure(cmd: list[str], marker: str) -> None:
    result = run(cmd)
    if result.returncode == 0:
        print(result.stdout)
        raise SystemExit(f'command unexpectedly accepted tampering: {cmd}')
    if marker not in result.stdout:
        print(result.stdout)
        raise SystemExit(f'tamper rejection did not contain expected marker {marker!r}')


def mutate_copy(base: Path, destination: Path, pattern: str, payload: bytes) -> Path:
    if destination.exists():
        shutil.rmtree(destination)
    shutil.copytree(base, destination)
    matches = sorted(destination.glob(pattern))
    if len(matches) != 1:
        raise SystemExit(f'expected exactly one {pattern}, found {matches}')
    with matches[0].open('ab') as stream:
        stream.write(payload)
    return destination


def main() -> int:
    if len(sys.argv) != 5:
        print('usage: production-tamper-gate-smoke.py <venom> <site> <out-root> <node>', file=sys.stderr)
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
    base = out_root / 'base'
    base.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(venom), 'build', str(site), '--out', str(base)], check=True)
    subprocess.run([str(venom), 'verify-runtime', str(base), '--target', 'browser', '--require-real-engine'], check=True)
    subprocess.run([node, str(harness), str(base), 'real-engine-strict', '--strict-no-source-eval', '--via-loader'], check=True)

    package_case = mutate_copy(base, out_root / 'tamper-package', 'assets/app/app.*.vbc', b'package-tamper')
    expect_failure(
        [node, str(harness), str(package_case), 'real-engine-strict', '--strict-no-source-eval', '--via-loader'],
        'package SHA-256 attestation mismatch',
    )

    quickjs_case = mutate_copy(base, out_root / 'tamper-quickjs-wasm', 'assets/runtime/runtime.*.wasm', b'quickjs-wasm-tamper')
    expect_failure(
        [node, str(harness), str(quickjs_case), 'real-engine-strict', '--strict-no-source-eval', '--via-loader'],
        'QuickJS WASM digest attestation mismatch',
    )

    verify_cases = [
        ('worker', 'assets/workers/worker.*.js', b'\n// worker tamper\n', 'bound asset hash mismatch: worker'),
        ('runtime-wasm', 'assets/runtime/rw.*.wasm', b'runtime-wasm-tamper', 'bound asset hash mismatch: runtime_wasm'),
        ('loader', 'assets/loader/loader.*.js', b'\n// loader tamper\n', 'loader subresource integrity mismatch'),
        ('style', 'assets/style/s.*.css', b'\n/* style tamper */\n', 'bound asset hash mismatch: style'),
    ]
    for label, pattern, payload, marker in verify_cases:
        case = mutate_copy(base, out_root / f'tamper-{label}', pattern, payload)
        expect_failure([str(venom), 'verify-runtime', str(case), '--target', 'browser', '--require-real-engine'], marker)

    print('v1.0.1 production tamper gate smoke: PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

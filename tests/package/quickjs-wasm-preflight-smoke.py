#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 3:
        print('usage: quickjs-wasm-preflight-smoke.py <repo-root> <out-dir>', file=sys.stderr)
        return 2
    root = Path(sys.argv[1]).resolve()
    out = Path(sys.argv[2]).resolve()
    out.mkdir(parents=True, exist_ok=True)
    manifest = out / 'quickjs-wasm-preflight.txt'
    report = out / 'quickjs-wasm-preflight.json'
    tool = root / 'tools' / 'quickjs_wasm_preflight.py'

    run = subprocess.run([
        sys.executable,
        str(tool),
        '--repo-root', str(root),
        '--allow-missing-emcc',
        '--manifest', str(manifest),
        '--json', str(report),
    ], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
    if run.returncode != 0:
        print(run.stdout)
        return run.returncode

    text = manifest.read_text(encoding='utf-8')
    for marker in (
        'VENOM_QJS_WASM_PREFLIGHT_V1',
        'runtime_abi=12',
        'package_version=83',
        'required_export_count=',
        'missing_required_export_count=0',
    ):
        if marker not in text:
            print(f'missing preflight manifest marker {marker!r}', file=sys.stderr)
            return 1

    info = json.loads(report.read_text(encoding='utf-8'))
    if info['package_version'] != 83 or info['runtime_abi'] != 12:
        print('unexpected preflight version fields', file=sys.stderr)
        return 1
    if not info['repo_inputs_ok']:
        print('repo input preflight should pass even when emcc is missing', file=sys.stderr)
        return 1
    if len(info['exports']) < len(info.get('required_probe_exports', [])):
        print('expected complete ABI12 probe export set', file=sys.stderr)
        return 1
    for name in ('venom_qjs_engine_abi', 'venom_qjs_execute_bytecode', 'venom_qjs_real_engine_candidate', 'venom_qjs_upstream_quickjs_ready'):
        if name not in info['exports']:
            print(f'missing ABI export {name}', file=sys.stderr)
            return 1

    script = root / 'scripts' / 'linux' / 'build-quickjs-wasm.sh'
    dry = subprocess.run([str(script), '--preflight-only', str(out / 'script-preflight')], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
    if dry.returncode != 0:
        print(dry.stdout)
        return dry.returncode
    if not (out / 'script-preflight' / 'quickjs-wasm-preflight.txt').exists():
        print('build-quickjs-wasm --preflight-only did not write preflight manifest', file=sys.stderr)
        return 1

    print('quickjs wasm preflight smoke: PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

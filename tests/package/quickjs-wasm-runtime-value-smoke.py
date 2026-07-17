#!/usr/bin/env python3
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 3:
        print('usage: quickjs-wasm-runtime-value-smoke.py <repo-root> <out-dir>', file=sys.stderr)
        return 2
    root = Path(sys.argv[1]).resolve()
    out = Path(sys.argv[2]).resolve()
    out.mkdir(parents=True, exist_ok=True)

    header = (root / 'src/generated/runtime/quickjs_runtime_wasm_blob.hpp').read_text(encoding='utf-8')
    body = header.split('inline constexpr std::uint8_t kQuickJsRuntimeWasmBlob[] = {', 1)[1].split('};', 1)[0]
    wasm = bytes(int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]{2})', body))
    wasm_path = out / 'quickjs-runtime-from-header.wasm'
    wasm_path.write_bytes(wasm)
    manifest = out / 'quickjs-runtime-values.manifest.txt'
    exports_json = out / 'quickjs-runtime-values.exports.json'

    proc = subprocess.run([
        sys.executable,
        str(root / 'tools' / 'wasm_exports.py'),
        str(wasm_path),
        '--require-from', str(root / 'src/runtime/quickjs_runtime_scaffold.c'),
        '--manifest', str(manifest),
        '--exports-json', str(exports_json),
        '--runtime-abi', '12',
        '--package-version', '83',
        '--validate-runtime-values',
        '--require-real-runtime-values',
        '--fail-missing',
    ], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)

    text = manifest.read_text(encoding='utf-8') if manifest.exists() else ''
    if proc.returncode != 0:
        print(proc.stdout)
        raise SystemExit('checked-in WASM blob must satisfy the v83 real-runtime value gate')
    for marker in (
        'VENOM_QJS_WASM_ARTIFACT_V3',
        'package_version=83',
        'runtime_values_checked=true',
        'runtime_values_satisfied=true',
        'venom_qjs_engine_version=83',
        'real_engine_candidate=true',
        'full_upstream_quickjs=true',
    ):
        if marker not in text:
            print(proc.stdout)
            raise SystemExit(f'missing runtime value marker {marker!r}')
    print('quickjs wasm runtime value smoke: PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

#!/usr/bin/env python3
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EXPECTED = 83
CHECKS = {
    'src/quickjs/include/venom/quickjs/abi.hpp': r'kRuntimePackageVersion\s*=\s*(\d+)u?;',
    'src/runtime/quickjs_runtime_scaffold.c': r'#define\s+QJS_VERSION\s+(\d+)u',
    'src/runtime/quickjs_upstream_runtime.c': r'#define\s+VENOM_QJS_RUNTIME_PACKAGE_VERSION\s+(\d+)u',
    'tools/build_emscripten.py': r'PACKAGE_VERSION\s*=\s*(\d+)',
    'tools/quickjs_wasm_cutover.py': r'PACKAGE_VERSION\s*=\s*(\d+)',
    'tools/quickjs_wasm_preflight.py': r'PACKAGE_VERSION\s*=\s*(\d+)',
    'tools/setup_emscripten.py': r'PACKAGE_VERSION\s*=\s*(\d+)',
    'tools/analyze_dist.py': r'PACKAGE_VERSION\s*=\s*(\d+)',
}

failures = []
for rel, pattern in CHECKS.items():
    path = ROOT / rel
    text = path.read_text(encoding='utf-8')
    match = re.search(pattern, text)
    if not match:
        failures.append(f'{rel}: missing version marker')
        continue
    value = int(match.group(1))
    if value != EXPECTED:
        failures.append(f'{rel}: expected {EXPECTED}, got {value}')

if failures:
    for failure in failures:
        print('[venom-version-smoke]', failure)
    sys.exit(1)

print(f'[venom-version-smoke] PASS package/runtime/tool version sources all match {EXPECTED}')

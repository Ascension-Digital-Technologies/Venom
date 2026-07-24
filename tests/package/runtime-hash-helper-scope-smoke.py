#!/usr/bin/env python3
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(sys.argv[1] if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]).resolve()
runtime = (root / 'src/generated/runtime/javascript/browser_runtime.js').read_text(encoding='utf-8')

helper_pos = runtime.find('function fnv1a32(bytes)')
seal_pos = runtime.find('function computeRuntimeIntegritySeal(opcodeMap = null)')
if helper_pos < 0:
    raise SystemExit('shared runtime template is missing fnv1a32')
if seal_pos < 0:
    raise SystemExit('shared runtime template is missing computeRuntimeIntegritySeal')
if helper_pos > seal_pos:
    raise SystemExit('fnv1a32 is declared after the integrity-seal code that consumes it')

patterns = [
    r'function fnv1a32\(bytes\) \{.*?\n\}',
    r'function runtimeIntegrityText\(value\) \{.*?\n\}',
    r'function computeRuntimeIntegritySeal\(opcodeMap = null\) \{.*?\n\}',
]
parts = []
for pattern in patterns:
    match = re.search(pattern, runtime, flags=re.S)
    if not match:
        raise SystemExit(f'cannot extract runtime helper: {pattern}')
    parts.append(match.group(0))

probe = """
const textEncoder = new TextEncoder();
let activeReleaseDiversification = Object.freeze({ build: 'probe' });
let activeTurboJsAbiFingerprint = Object.freeze({ hash: 123, runtimeAbi: 12 });
""" + '\n'.join(parts) + """
const value = computeRuntimeIntegritySeal(null);
if (!Number.isInteger(value) || value === 0) throw new Error('invalid runtime integrity hash');
console.log('runtime hash helper probe: PASS');
"""

with tempfile.TemporaryDirectory() as td:
    script = Path(td) / 'probe.mjs'
    script.write_text(probe, encoding='utf-8')
    completed = subprocess.run(['node', str(script)], text=True, capture_output=True)
    if completed.returncode != 0:
        raise SystemExit('runtime hash helper execution failed:\n' + completed.stdout + completed.stderr)

print('runtime hash helper scope smoke: PASS')

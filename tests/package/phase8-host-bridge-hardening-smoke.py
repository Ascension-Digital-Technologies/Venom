#!/usr/bin/env python3
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]

def project_version(text):
    match = re.search(r'project\(venom\s+VERSION\s+(\d+)\.(\d+)\.(\d+)', text, re.S)
    return tuple(map(int, match.groups())) if match else (0, 0, 0)
runtime = (root / 'src/generated/runtime/runtime_js.cpp').read_text()
runtime += (root / 'src/runtime/templates/runtime.js').read_text(encoding='utf-8')
cmake = (root / 'CMakeLists.txt').read_text()
cli = (root / 'src/compiler/commands/cli.cpp').read_text()
checks = {
    'version': project_version(cmake) >= (1, 0, 22),
    'generation handles': 'createDomHandleRegistry' in runtime and 'stale DOM handle' in runtime,
    'ownership checks': 'DOM handle ownership denied' in runtime,
    'bounded handles': 'DOM handle capacity exceeded' in runtime and '8192' in runtime,
    'unsafe sinks': 'unsafe DOM attribute denied' in runtime and "attr.startsWith('on')" in runtime,
    'url normalization': 'normalizeHostUrl' in runtime and 'cross-origin host request denied' in runtime,
    'fetch hardening': "credentials: 'same-origin'" in runtime and "redirect: 'error'" in runtime,
    'no inline host eval': 'Protected runtime never evaluates inline event source in the host realm.' in runtime,
    'route revocation': "advanceDomGeneration" in runtime,
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('phase8 checks failed: ' + ', '.join(failed))
print('phase8 host bridge hardening smoke passed')

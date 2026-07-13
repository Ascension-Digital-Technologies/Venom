#!/usr/bin/env python3

def project_version(text):
    match = re.search(r'project\(venom\s+VERSION\s+(\d+)\.(\d+)\.(\d+)', text, re.S)
    return tuple(map(int, match.groups())) if match else (0, 0, 0)
import sys
from pathlib import Path
import re
r=Path(sys.argv[1])
cm=(r/'CMakeLists.txt').read_text(); opts=(r/'cmake/options.cmake').read_text(); bench=(r/'tools/benchmark_runtime.py').read_text(); wasm=(r/'scripts/build-quickjs-wasm.sh').read_text(); cli=(r/'src/compiler/cli.cpp').read_text()
checks={
 'version': project_version(cm) >= (1, 0, 24),
 'ipo':'VENOM_ENABLE_IPO' in opts and 'check_ipo_supported' in cm,
 'pgo_modes':'VENOM_PGO_MODE' in opts and '-fprofile-generate' in cm and '-fprofile-use' in cm,
 'pgo_scripts':(r/'scripts/build-pgo.sh').exists() and (r/'scripts/build-pgo.ps1').exists(),
 'benchmark_v2':"venom.performance.v2" in bench and '--baseline' in bench and '--budget' in bench,
 'percentiles':'p95_ms' in bench and 'median_ms' in bench,
 'memory':'heap_bytes' in bench,
 'wasm_profiles':all(x in wasm for x in ('size)','balanced)','performance)')),
}
failed=[k for k,v in checks.items() if not v]
for k,v in checks.items(): print(f"[{'PASS' if v else 'FAIL'}] {k}")
if failed: raise SystemExit('missing: '+', '.join(failed))

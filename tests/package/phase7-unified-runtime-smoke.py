#!/usr/bin/env python3
from pathlib import Path
import re
root=Path(__file__).resolve().parents[2]

def project_version(text):
    match = re.search(r'project\(venom\s+VERSION\s+(\d+)\.(\d+)\.(\d+)', text, re.S)
    return tuple(map(int, match.groups())) if match else (0, 0, 0)
build=(root/'src/pipeline/build.cpp').read_text() + (root/'src/pipeline/build_package_metadata.cpp').read_text() + (root/'src/pipeline/build_runtime_metadata.cpp').read_text() + (root/'src/pipeline/build_runtime_audit_metadata.cpp').read_text() + (root/'src/pipeline/build_runtime_module_metadata.cpp').read_text()
runtime=(root/'src/generated/runtime/runtime_js.cpp').read_text()
runtime += (root / 'src/generated/runtime/javascript/browser_runtime.js').read_text(encoding='utf-8')
engine=(root/'src/generated/runtime/quickjs_engine_module.cpp').read_text()
abi=(root/'tools/quickjs_release_abi.py').read_text()
cli=(root/'src/cli/cli.cpp').read_text()
checks={
 'version': project_version((root/'CMakeLists.txt').read_text()) >= (1, 0, 20),
 'fingerprint section':'quickjs-abi-fingerprint.vqaf' in build,
 'fingerprint parser':'parseQuickJsAbiFingerprint' in runtime,
 'fingerprint bind':'package/runtime ABI fingerprint mismatch' in engine,
 'strict exports':'validateReleaseAbiExports(module)' in engine and 'unapproved' in engine,
 'mandatory limits':'release runtime lacks execution-limit support' in engine,
 'mandatory compact':'compact result descriptor is required' in engine,
 'no old compatibility':'pre-v1.0.15 embedded WASM' not in engine,
 '23 function ABI':abi.count("'venom_qjs_") == 23,
}
for k,v in checks.items(): print(f"[{'PASS' if v else 'FAIL'}] {k}")
raise SystemExit(0 if all(checks.values()) else 1)

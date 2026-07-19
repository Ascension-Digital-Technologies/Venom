#!/usr/bin/env python3
from __future__ import annotations
import json, subprocess, sys, tempfile
from pathlib import Path

root=Path(__file__).resolve().parents[2]
workflow=(root/'.github/workflows/release.yml').read_text(encoding='utf-8')
build=(root / 'src/pipeline/build.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_package_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_audit_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_module_metadata.cpp').read_text(encoding='utf-8') + (root/'src/pipeline/build_support.cpp').read_text(encoding='utf-8')
runtime=(root/'src/generated/runtime/runtime_js.cpp').read_text(encoding='utf-8') + (root/'src/generated/runtime/javascript/browser_runtime.js').read_text(encoding='utf-8')
engine=(root/'src/generated/runtime/quickjs_engine_module.cpp').read_text(encoding='utf-8')
assert 'verified-runtime:' in workflow
assert 'needs: [verified-runtime, release-closure]' in workflow
assert 'needs: [verified-runtime, release-closure, package]' in workflow
assert '--verified-runtime' in workflow and '--require-runtime' in workflow
for key in ('contract_only', 'scaffold_runtime', 'real_engine_candidate', 'full_upstream_quickjs', 'required_exports_satisfied'):
    assert f'require_value("{key}"' in build
assert '__VENOM_RUNTIME_ENGINE_FALLBACK_BLOCK__' in runtime
assert '__VENOM_RUNTIME_LEGACY_FALLBACK_BLOCK__' in runtime
assert '__VENOM_QUICKJS_FALLBACK_BLOCK__' in engine
assert 'erase_once("      const fn = new Function' not in runtime
assert 'protected QuickJS module fallback block marker missing' not in engine
assert "unknownImport: plan.values.get('unknown_import') || 'deny'" in runtime
assert "unknownHostCall: plan.values.get('unknown_host_call') || 'deny'" in runtime

with tempfile.TemporaryDirectory() as td:
    d=Path(td)
    pkg=d/'venom-v2.0.0-linux.zip'; pkg.write_bytes(b'package')
    evidence=d/'evidence.zip'; evidence.write_bytes(b'evidence')
    wasm=d/'quickjs-runtime.wasm'; wasm.write_bytes(b'\0asmcanonical-runtime')
    out=d/'set'
    cmd=[sys.executable,str(root/'tools/package_release_set.py'),'--version','2.0.0','--source-revision','test','--packages',str(pkg),'--compatibility-evidence',str(evidence),'--verified-runtime',str(wasm),'--out',str(out),'--source-date-epoch','1704067200']
    subprocess.run(cmd,check=True,capture_output=True,text=True)
    obj=json.loads((out/'RELEASE_SET.json').read_text())
    assert obj['verified_runtime']['name']=='quickjs-runtime.wasm'
    verify=subprocess.run([sys.executable,str(root/'tools/verify_release_set.py'),str(out),'--expect-version','2.0.0','--require-evidence','--require-runtime'],capture_output=True,text=True)
    assert verify.returncode==0, verify.stdout+verify.stderr
print('verified runtime release chain smoke: PASS')

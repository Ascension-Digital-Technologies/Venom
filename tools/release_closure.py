#!/usr/bin/env python3
"""Verify that a Venom source tree is ready to produce a browser-complete release."""
from __future__ import annotations
import argparse, hashlib, json, re, subprocess, sys
from pathlib import Path

EXPECTED_EXPORTS = {
    'venom_qjs_context_alloc','venom_qjs_context_free','venom_qjs_context_set_limits',
    'venom_qjs_context_set_execution_limits','venom_qjs_script_buffer_alloc',
    'venom_qjs_script_buffer_capacity','venom_qjs_script_buffer_set_expected_hash',
    'venom_qjs_script_buffer_free','venom_qjs_bytecode_validate','venom_qjs_execute_bytecode',
    'venom_qjs_compact_result_ptr','venom_qjs_diversification_install',
    'venom_qjs_exception_message_ptr','venom_qjs_exception_message_size',
    'venom_qjs_exception_clear','venom_qjs_upstream_quickjs_ready',
    'venom_qjs_bridge_abi','venom_qjs_bridge_input_alloc','venom_qjs_bridge_input_capacity',
    'venom_qjs_bridge_invoke','venom_qjs_bridge_result_ptr','venom_qjs_bridge_result_size',
    'venom_qjs_bridge_release',
}

def sha256(path: Path) -> str:
    h=hashlib.sha256(); h.update(path.read_bytes()); return h.hexdigest()

def kv(text: str) -> dict[str,str]:
    out={}
    for line in text.splitlines():
        if '=' in line and not line.lstrip().startswith('#'):
            k,v=line.split('=',1); out[k.strip()]=v.strip()
    return out

def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument('--require-real', action='store_true')
    ap.add_argument('--json-out', type=Path)
    args=ap.parse_args(); root=args.repo_root.resolve()
    failures=[]; warnings=[]
    header=root/'include/venom/generated/runtime/quickjs_runtime_wasm_blob.hpp'
    abi_tool=root/'tools/quickjs_release_abi.py'
    lock_path=root/'toolchains.lock.json'
    cmake=root/'CMakeLists.txt'
    if not header.exists(): failures.append('embedded QuickJS WASM header is missing')
    info={}
    if header.exists():
        text=header.read_text(encoding='utf-8')
        m=re.search(r'kQuickJsRuntimeWasmBlobProvenance\s*=\s*R"[^\(]*\((.*?)\)[A-Za-z0-9_]*"', text, re.S)
        if not m: failures.append('embedded QuickJS WASM provenance is missing')
        else: info=kv(m.group(1))
    expected={
      'runtime_abi':'12','artifact_kind':'upstream-quickjs-wasm',
      'runtime_implementation':'quickjs-wasm-upstream-quickjs','contract_only':'false',
      'scaffold_runtime':'false','real_engine_candidate':'true','full_upstream_quickjs':'true',
      'required_exports_satisfied':'true','missing_export_count':'0','bytecode_format':'VQJSBC03',
      'module_bundle_contract':'VQJSMB04','literal_dynamic_import':'true',
      'native_object_reader':'JS_ReadObject','source_materialization':'false',
      'venom_qjs_wasm_native_stack_capacity':'4194304',
    }
    real_ok=True
    for k,v in expected.items():
        if info.get(k)!=v:
            real_ok=False
            if args.require_real: failures.append(f'embedded runtime {k} must be {v!r}, got {info.get(k, "")!r}')
    if info.get('finish_blocker','') not in ('','none'):
        real_ok=False
        if args.require_real: failures.append('embedded runtime still declares a finish blocker')
    release_exports=[]
    if abi_tool.exists():
        text=abi_tool.read_text(encoding='utf-8')
        release_exports=re.findall(r"'((?:venom_qjs_)[A-Za-z0-9_]+)'", text)
        if set(release_exports)!=EXPECTED_EXPORTS:
            failures.append('release ABI allowlist differs from the 23-export production contract')
        if len(release_exports)!=23 or len(set(release_exports))!=23:
            failures.append('release ABI must contain exactly 23 unique exports')
    else: failures.append('release ABI generator is missing')
    lock={}
    if lock_path.exists():
        try: lock=json.loads(lock_path.read_text(encoding='utf-8'))
        except Exception as e: failures.append(f'invalid toolchains.lock.json: {e}')
    else: failures.append('toolchains.lock.json is missing')
    em=lock.get('emscripten',{}) if isinstance(lock,dict) else {}
    version=str(em.get('version',''))
    if not version or version=='latest': failures.append('stable release requires a pinned Emscripten version')
    cmake_version=''
    if cmake.exists():
        m=re.search(r'project\(venom\s+VERSION\s+([0-9.]+)',cmake.read_text(encoding='utf-8'),re.S)
        cmake_version=m.group(1) if m else ''
    report={
      'schema':'VENOM_RELEASE_CLOSURE_V1','version':cmake_version,'strict':args.require_real,
      'embedded_runtime_real':real_ok,'embedded_runtime_sha256':sha256(header) if header.exists() else '',
      'release_abi_export_count':len(release_exports),'emscripten_version':version,
      'failures':failures,'warnings':warnings,'ready':not failures,
    }
    if args.json_out:
        args.json_out.parent.mkdir(parents=True,exist_ok=True)
        args.json_out.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(f"[venom] release closure: {'PASS' if not failures else 'FAIL'}")
    print(f"[venom] embedded real runtime: {str(real_ok).lower()}")
    print(f"[venom] release ABI exports: {len(release_exports)}")
    print(f"[venom] pinned Emscripten: {version or 'missing'}")
    for f in failures: print(f'[venom] failure: {f}',file=sys.stderr)
    return 0 if not failures else 1
if __name__=='__main__': raise SystemExit(main())

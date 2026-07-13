#!/usr/bin/env python3
"""Inspect WebAssembly exports and verify Venom QuickJS ABI export coverage.

The export section alone is not enough to prove a real/current QuickJS runtime:
a stale or scaffold-shaped module can expose the same function names. Use
`--validate-runtime-values` in cutover/release paths to instantiate the module
with Node and call the ABI truth exports.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

STANDALONE_WASM_SUPPORT_EXPORTS = {
    'memory',
    '__indirect_function_table',
    '_emscripten_stack_restore',
    'emscripten_stack_get_current',
}

RUNTIME_VALUE_EXPORTS = (
    'venom_qjs_engine_abi',
    'venom_qjs_engine_version',
    'venom_qjs_wasm_native_stack_capacity',
    'venom_qjs_real_engine_candidate',
    'venom_qjs_fallback_required',
    'venom_qjs_upstream_quickjs_ready',
)


def read_u32_leb(data: bytes, pos: int) -> tuple[int, int]:
    result = 0
    shift = 0
    while True:
        if pos >= len(data):
            raise ValueError('unexpected EOF while reading LEB128')
        byte = data[pos]
        pos += 1
        result |= (byte & 0x7F) << shift
        if (byte & 0x80) == 0:
            return result, pos
        shift += 7
        if shift > 35:
            raise ValueError('LEB128 value is too large')


def read_name(data: bytes, pos: int) -> tuple[str, int]:
    size, pos = read_u32_leb(data, pos)
    end = pos + size
    if end > len(data):
        raise ValueError('unexpected EOF while reading name')
    return data[pos:end].decode('utf-8', errors='replace'), end


def wasm_exports(data: bytes) -> list[str]:
    if not data.startswith(b'\0asm'):
        raise ValueError('input is not a WebAssembly module')
    if len(data) < 8:
        raise ValueError('truncated WebAssembly header')
    pos = 8
    exports: list[str] = []
    while pos < len(data):
        section_id = data[pos]
        pos += 1
        section_size, pos = read_u32_leb(data, pos)
        section_end = pos + section_size
        if section_end > len(data):
            raise ValueError('section extends beyond end of file')
        if section_id == 7:
            count, p = read_u32_leb(data, pos)
            for _ in range(count):
                name, p = read_name(data, p)
                if p >= section_end:
                    raise ValueError('truncated export entry')
                _kind = data[p]
                p += 1
                _index, p = read_u32_leb(data, p)
                exports.append(name)
            break
        pos = section_end
    return exports


def c_required_exports(path: Path) -> list[str]:
    text = path.read_text(encoding='utf-8')
    return sorted(set(re.findall(r'VENOM_QJS_PUBLIC\("([A-Za-z0-9_]+)"\)', text)))


def call_runtime_values_with_node(wasm: Path, node: str, names: list[str] | None = None) -> tuple[dict[str, int | str | None], list[str]]:
    node_bin = shutil.which(node) or (str(Path(node)) if Path(node).exists() else '')
    if not node_bin:
        return {}, [f'node executable not found: {node}']
    names = names or list(RUNTIME_VALUE_EXPORTS)
    script = r'''
const fs = require('fs');
const wasm = process.argv[2];
const names = process.argv.slice(3);

function makeTable() {
  try { return new WebAssembly.Table({ initial: 1, element: 'funcref' }); }
  catch (_) { return new WebAssembly.Table({ initial: 1, element: 'anyfunc' }); }
}

function defaultImportFor(desc) {
  if (desc.kind === 'function') return function () { return 0; };
  if (desc.kind === 'memory') return new WebAssembly.Memory({ initial: 512, maximum: 4096 });
  if (desc.kind === 'table') return makeTable();
  if (desc.kind === 'global') return new WebAssembly.Global({ value: 'i32', mutable: true }, 0);
  return 0;
}

function makeImportObject(module) {
  const imports = {};
  for (const desc of WebAssembly.Module.imports(module)) {
    if (!imports[desc.module]) imports[desc.module] = {};
    // Runtime value probes call only constant metadata exports, so unresolved
    // host shims can safely be inert. This keeps the verification focused on
    // ABI/package truth values instead of requiring the full browser host bridge.
    imports[desc.module][desc.name] = defaultImportFor(desc);
  }
  return imports;
}

(async () => {
  const bytes = fs.readFileSync(wasm);
  const out = {};
  const errors = [];
  let instance;
  try {
    const module = await WebAssembly.compile(bytes);
    const importObject = makeImportObject(module);
    instance = await WebAssembly.instantiate(module, importObject);
  } catch (err) {
    console.log(JSON.stringify({ values: out, errors: ['instantiate: ' + (err && err.message ? err.message : String(err))] }));
    return;
  }
  for (const name of names) {
    const fn = instance.exports[name];
    if (typeof fn !== 'function') {
      out[name] = null;
      errors.push('missing callable export: ' + name);
      continue;
    }
    try {
      const value = fn();
      out[name] = typeof value === 'bigint' ? Number(value) : value;
    } catch (err) {
      out[name] = 'error';
      errors.push(name + ': ' + (err && err.message ? err.message : String(err)));
    }
  }
  console.log(JSON.stringify({ values: out, errors }));
})().catch((err) => {
  console.log(JSON.stringify({ values: {}, errors: ['node fatal: ' + (err && err.message ? err.message : String(err))] }));
});
'''
    with tempfile.NamedTemporaryFile('w', suffix='.js', delete=False, encoding='utf-8') as tmp:
        tmp.write(script)
        tmp_path = Path(tmp.name)
    try:
        proc = subprocess.run(
            [node_bin, str(tmp_path), str(wasm), *names],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=30,
        )
    except subprocess.TimeoutExpired:
        return {}, ['node runtime value probe timed out']
    finally:
        try:
            tmp_path.unlink()
        except OSError:
            pass
    try:
        report = json.loads(proc.stdout.strip().splitlines()[-1] if proc.stdout.strip() else '{}')
    except json.JSONDecodeError:
        return {}, ['node runtime value probe did not return JSON: ' + proc.stdout[:200]]
    values = report.get('values') if isinstance(report, dict) else {}
    errors = report.get('errors') if isinstance(report, dict) else []
    if proc.returncode != 0:
        errors = list(errors or []) + [f'node exited with {proc.returncode}']
    return (values if isinstance(values, dict) else {}), [str(e) for e in (errors or [])]


def runtime_value_failures(values: dict[str, Any], *, runtime_abi: int, package_version: int, require_real: bool) -> list[str]:
    failures: list[str] = []
    if values.get('venom_qjs_engine_abi') != runtime_abi:
        failures.append(f'venom_qjs_engine_abi expected {runtime_abi}, got {values.get("venom_qjs_engine_abi")!r}')
    if values.get('venom_qjs_engine_version') != package_version:
        failures.append(f'venom_qjs_engine_version expected {package_version}, got {values.get("venom_qjs_engine_version")!r}')
    native_stack = values.get('venom_qjs_wasm_native_stack_capacity')
    if not isinstance(native_stack, int) or native_stack < 2 * 1024 * 1024:
        failures.append(f'venom_qjs_wasm_native_stack_capacity expected at least 2097152, got {native_stack!r}')
    if require_real:
        if values.get('venom_qjs_real_engine_candidate') != 1:
            failures.append(f'venom_qjs_real_engine_candidate expected 1, got {values.get("venom_qjs_real_engine_candidate")!r}')
        if values.get('venom_qjs_fallback_required') != 0:
            failures.append(f'venom_qjs_fallback_required expected 0, got {values.get("venom_qjs_fallback_required")!r}')
        if values.get('venom_qjs_upstream_quickjs_ready') != 1:
            failures.append(f'venom_qjs_upstream_quickjs_ready expected 1, got {values.get("venom_qjs_upstream_quickjs_ready")!r}')
    return failures


def manifest_text(
    *,
    wasm: Path,
    exports: list[str],
    required: list[str],
    missing: list[str],
    runtime_abi: int,
    package_version: int,
    runtime_values: dict[str, Any],
    runtime_value_errors: list[str],
    runtime_value_checked: bool,
) -> str:
    data = wasm.read_bytes()
    has_required_shape = bool(required) and not missing
    runtime_values_ok = (not runtime_value_checked) or not runtime_value_errors
    ok = has_required_shape and runtime_values_ok
    lines = [
        'VENOM_QJS_WASM_ARTIFACT_V3',
        'version=3',
        f'runtime_abi={runtime_abi}',
        f'package_version={package_version}',
        'artifact_kind=upstream-quickjs-wasm' if ok else ('artifact_kind=upstream-quickjs-wasm-incomplete' if required else 'artifact_kind=wasm-export-report'),
        'runtime_implementation=quickjs-wasm-upstream-quickjs' if ok else ('runtime_implementation=quickjs-wasm-upstream-quickjs-incomplete' if required else 'runtime_implementation=unknown-wasm-module'),
        'runtime_claim=full-upstream-quickjs-wasm' if ok else ('runtime_claim=incomplete-upstream-quickjs-wasm' if required else 'runtime_claim=export-report-only'),
        'contract_only=false' if ok else 'contract_only=true',
        'scaffold_runtime=false' if ok else 'scaffold_runtime=unknown',
        'real_engine_candidate=true' if ok else 'real_engine_candidate=false',
        'full_upstream_quickjs=true' if ok else 'full_upstream_quickjs=false',
        'fallback_required=false' if ok else 'fallback_required=unknown',
        'finish_blocker=none' if ok else ('finish_blocker=missing_required_wasm_exports_or_runtime_value_mismatch' if required else 'finish_blocker=no_required_abi_export_set_supplied'),
        f'artifact={wasm.name}',
        f'wasm_sha256={hashlib.sha256(data).hexdigest()}',
        f'wasm_bytes={len(data)}',
        f'export_count={len(exports)}',
        f'required_export_count={len(required)}',
        f'missing_export_count={len(missing)}',
        'required_exports_satisfied=true' if bool(required) and not missing else 'required_exports_satisfied=false',
        'runtime_values_checked=true' if runtime_value_checked else 'runtime_values_checked=false',
        'runtime_values_satisfied=true' if runtime_value_checked and not runtime_value_errors else ('runtime_values_satisfied=not-checked' if not runtime_value_checked else 'runtime_values_satisfied=false'),
    ]
    if runtime_value_checked:
        for name in RUNTIME_VALUE_EXPORTS:
            lines.append(f'{name}={runtime_values.get(name, "")}')
    if missing:
        lines.append('missing_exports=' + ','.join(missing))
    else:
        lines.append('missing_exports=')
    if runtime_value_errors:
        lines.append('runtime_value_errors=' + '|'.join(runtime_value_errors))
    else:
        lines.append('runtime_value_errors=')
    lines.append('exports=' + ','.join(exports))
    return '\n'.join(lines) + '\n'


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('wasm', type=Path)
    ap.add_argument('--require-from', type=Path, help='C source containing __attribute__((export_name(...))) declarations')
    ap.add_argument('--require-json', type=Path, help='JSON array of required export names; leading underscores are normalized')
    ap.add_argument('--exact-exports', action='store_true', help='require the module export set to exactly match the required set plus memory')
    ap.add_argument('--release-runtime-values', action='store_true', help='validate only the minimal release truth export')
    ap.add_argument('--exports-json', type=Path)
    ap.add_argument('--manifest', type=Path)
    ap.add_argument('--runtime-abi', type=int, default=12)
    ap.add_argument('--package-version', type=int, default=83)
    ap.add_argument('--validate-runtime-values', action='store_true', help='instantiate the module and verify ABI/version/truth exports')
    ap.add_argument('--node', default='node', help='Node.js executable used for --validate-runtime-values')
    ap.add_argument('--require-real-runtime-values', action='store_true', help='with --validate-runtime-values, require real-engine truth exports')
    ap.add_argument('--fail-missing', action='store_true')
    args = ap.parse_args()

    data = args.wasm.read_bytes()
    exports = sorted(set(wasm_exports(data)))
    required = c_required_exports(args.require_from) if args.require_from else []
    if args.require_json:
        raw_required = json.loads(args.require_json.read_text(encoding='utf-8'))
        if not isinstance(raw_required, list) or not all(isinstance(x, str) for x in raw_required):
            raise SystemExit('--require-json must contain a JSON string array')
        required = sorted({x[1:] if x.startswith('_') else x for x in raw_required})
    export_names = set(exports)
    missing = []
    for name in required:
        if name not in export_names and ('_' + name) not in export_names:
            missing.append(name)

    exact_extra: list[str] = []
    if args.exact_exports and required:
        # Emscripten STANDALONE_WASM emits a small support surface even when
        # EXPORTED_FUNCTIONS contains only the release ABI. Keep exactness for
        # Venom exports while tolerating these documented toolchain exports.
        allowed = set(required) | STANDALONE_WASM_SUPPORT_EXPORTS
        exact_extra = sorted(name for name in exports if name not in allowed)
        if exact_extra:
            missing.extend('unexpected:' + name for name in exact_extra)

    runtime_values: dict[str, Any] = {}
    runtime_value_errors: list[str] = []
    if args.validate_runtime_values:
        if args.release_runtime_values:
            runtime_values, probe_errors = call_runtime_values_with_node(args.wasm, args.node, names=['venom_qjs_upstream_quickjs_ready'])
            runtime_value_errors.extend(probe_errors)
            if runtime_values.get('venom_qjs_upstream_quickjs_ready') != 1:
                runtime_value_errors.append('venom_qjs_upstream_quickjs_ready expected 1, got %r' % runtime_values.get('venom_qjs_upstream_quickjs_ready'))
        else:
            runtime_values, probe_errors = call_runtime_values_with_node(args.wasm, args.node)
            runtime_value_errors.extend(probe_errors)
            runtime_value_errors.extend(runtime_value_failures(
                runtime_values,
                runtime_abi=args.runtime_abi,
                package_version=args.package_version,
                require_real=args.require_real_runtime_values,
            ))

    if args.exports_json:
        args.exports_json.parent.mkdir(parents=True, exist_ok=True)
        args.exports_json.write_text(json.dumps({
            'wasm': str(args.wasm),
            'sha256': hashlib.sha256(data).hexdigest(),
            'exports': exports,
            'required_exports': required,
            'missing_exports': missing,
            'unexpected_exports': exact_extra,
            'required_exports_satisfied': bool(required) and not missing,
            'runtime_values_checked': args.validate_runtime_values,
            'runtime_values': runtime_values,
            'runtime_value_errors': runtime_value_errors,
            'runtime_values_satisfied': args.validate_runtime_values and not runtime_value_errors,
        }, indent=2), encoding='utf-8')

    if args.manifest:
        args.manifest.parent.mkdir(parents=True, exist_ok=True)
        args.manifest.write_text(manifest_text(
            wasm=args.wasm,
            exports=exports,
            required=required,
            missing=missing,
            runtime_abi=args.runtime_abi,
            package_version=args.package_version,
            runtime_values=runtime_values,
            runtime_value_errors=runtime_value_errors,
            runtime_value_checked=args.validate_runtime_values,
        ), encoding='utf-8')

    print(f'[venom] wasm exports: {len(exports)}')
    if required:
        print(f'[venom] required ABI exports: {len(required)}')
        print(f'[venom] missing ABI exports: {len(missing)}')
        if missing:
            print('[venom] missing: ' + ', '.join(missing[:32]) + (' ...' if len(missing) > 32 else ''), file=sys.stderr)
    if args.validate_runtime_values:
        print(f'[venom] runtime value checks: {"PASS" if not runtime_value_errors else "FAIL"}')
        if runtime_value_errors:
            for err in runtime_value_errors[:16]:
                print(f'[venom] runtime value error: {err}', file=sys.stderr)
    if (missing or runtime_value_errors) and args.fail_missing:
        return 2
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

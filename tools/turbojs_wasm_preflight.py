#!/usr/bin/env python3
"""Preflight the upstream TurboJS WASM cutover toolchain and ABI inputs.

This tool is intentionally safe to run on machines without Emscripten. By

default it reports missing `emcc` as a setup problem with exit code 3. Use
`--allow-missing-emcc` in tests/CI jobs that only want to validate repository
cutover inputs without requiring the WASM toolchain to be installed.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

PACKAGE_VERSION = 83
RUNTIME_ABI = 12
MIN_REQUIRED_EXPORTS = 16
REQUIRED_EXPORT_NAMES = (
    'venom_tjs_engine_abi',
    'venom_tjs_engine_version',
    'venom_tjs_wasm_native_stack_capacity',
    'venom_tjs_context_alloc',
    'venom_tjs_context_free',
    'venom_tjs_context_set_limits',
    'venom_tjs_script_buffer_alloc',
    'venom_tjs_script_buffer_ptr',
    'venom_tjs_script_buffer_capacity',
    'venom_tjs_execute_bytecode',
    'venom_tjs_result_ptr',
    'venom_tjs_result_size',
    'venom_tjs_exception_ptr',
    'venom_tjs_exception_size',
    'venom_tjs_real_engine_candidate',
    'venom_tjs_upstream_turbojs_ready',
    'venom_tjs_bridge_abi',
    'venom_tjs_bridge_invoke',
    'venom_tjs_bridge_result_ptr',
    'venom_tjs_bridge_result_size',
)
REQUIRED_SOURCES = (
    'src/runtime/turbojs_runtime_scaffold.c',
    'third_party/turbojs/turbojs.c',
    'third_party/turbojs/turbojs-libc.c',
    'third_party/turbojs/libregexp.c',
    'third_party/turbojs/libunicode.c',
    'third_party/turbojs/dtoa.c',
    'third_party/turbojs/turbojs.h',
    'tools/wasm_exports.py',
    'tools/embed_wasm.py',
)


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def resolve_emcc(requested: str | None) -> str | None:
    candidate = requested or os.environ.get('EMCC') or 'emcc'
    return shutil.which(candidate) or (candidate if Path(candidate).exists() else None)


def command_for_tool(exe: str, args: list[str] | None = None) -> list[str]:
    args = args or []
    if os.name == 'nt' and exe.lower().endswith(('.bat', '.cmd')):
        return ['cmd.exe', '/d', '/s', '/c', subprocess.list2cmdline([exe, *args])]
    return [exe, *args]


def emcc_version(emcc: str) -> str:
    try:
        proc = subprocess.run(command_for_tool(emcc, ['--version']), text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=20)
    except Exception as exc:  # pragma: no cover - platform/toolchain dependent
        return f'unavailable: {exc}'
    first = proc.stdout.splitlines()[0] if proc.stdout else ''
    return first.strip() or f'exit={proc.returncode}'


def collect_exports(scaffold: Path) -> list[str]:
    text = scaffold.read_text(encoding='utf-8')
    return sorted(set(re.findall(r'VENOM_TJS_PUBLIC\("([A-Za-z0-9_]+)"\)', text)))


def manifest(info: dict) -> str:
    missing_sources = ','.join(info['missing_sources'])
    missing_exports = ','.join(info['missing_required_exports'])
    lines = [
        'VENOM_TJS_WASM_PREFLIGHT_V1',
        'version=1',
        f'runtime_abi={RUNTIME_ABI}',
        f'package_version={PACKAGE_VERSION}',
        f'status={info["status"]}',
        f'emcc_found={str(info["emcc_found"]).lower()}',
        f'emcc_path={info.get("emcc_path") or ""}',
        f'emcc_version={info.get("emcc_version") or ""}',
        f'source_count={info["source_count"]}',
        f'missing_source_count={len(info["missing_sources"])}',
        f'missing_sources={missing_sources}',
        f'required_export_count={len(info["exports"])}',
        f'min_required_exports={MIN_REQUIRED_EXPORTS}',
        f'missing_required_export_count={len(info["missing_required_exports"])}',
        f'missing_required_exports={missing_exports}',
        f'turbojs_scaffold_sha256={info.get("turbojs_scaffold_sha256", "")}',
        'cutover_ready=true' if info['cutover_ready'] else 'cutover_ready=false',
    ]
    return '\n'.join(lines) + '\n'


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument('--emcc', help='emcc executable path/name; defaults to EMCC env var or emcc')
    ap.add_argument('--allow-missing-emcc', action='store_true', help='return success when repo inputs are valid but emcc is not installed')
    ap.add_argument('--manifest', type=Path, help='write text preflight manifest')
    ap.add_argument('--json', dest='json_path', type=Path, help='write JSON preflight report')
    args = ap.parse_args()

    root = args.repo_root.resolve()
    scaffold = root / 'src/runtime/turbojs_runtime_scaffold.c'
    missing_sources = [rel for rel in REQUIRED_SOURCES if not (root / rel).exists()]
    exports = collect_exports(scaffold) if scaffold.exists() else []
    export_set = set(exports)
    missing_required_exports = [name for name in REQUIRED_EXPORT_NAMES if name not in export_set]
    emcc = resolve_emcc(args.emcc)
    emcc_found = bool(emcc)
    repo_inputs_ok = not missing_sources and not missing_required_exports and len(exports) >= MIN_REQUIRED_EXPORTS
    cutover_ready = repo_inputs_ok and emcc_found
    status = 'ready' if cutover_ready else ('missing-emcc' if repo_inputs_ok and not emcc_found else 'repo-inputs-incomplete')

    info = {
        'version': 1,
        'runtime_abi': RUNTIME_ABI,
        'package_version': PACKAGE_VERSION,
        'status': status,
        'repo_root': str(root),
        'emcc_found': emcc_found,
        'emcc_path': emcc or '',
        'emcc_version': emcc_version(emcc) if emcc else '',
        'source_count': len(REQUIRED_SOURCES),
        'missing_sources': missing_sources,
        'exports': exports,
        'required_probe_exports': list(REQUIRED_EXPORT_NAMES),
        'missing_required_exports': missing_required_exports,
        'min_required_exports': MIN_REQUIRED_EXPORTS,
        'turbojs_scaffold_sha256': sha256_file(scaffold) if scaffold.exists() else '',
        'repo_inputs_ok': repo_inputs_ok,
        'cutover_ready': cutover_ready,
    }

    if args.manifest:
        args.manifest.parent.mkdir(parents=True, exist_ok=True)
        args.manifest.write_text(manifest(info), encoding='utf-8')
    if args.json_path:
        args.json_path.parent.mkdir(parents=True, exist_ok=True)
        args.json_path.write_text(json.dumps(info, indent=2), encoding='utf-8')

    print(f'[venom] TurboJS WASM preflight status: {status}')
    print(f'[venom] ABI exports discovered: {len(exports)}')
    print(f'[venom] emcc: {emcc or "not found"}')
    if missing_sources:
        print('[venom] missing sources: ' + ', '.join(missing_sources), file=sys.stderr)
    if missing_required_exports:
        print('[venom] missing required exports: ' + ', '.join(missing_required_exports), file=sys.stderr)

    if not repo_inputs_ok:
        return 2
    if not emcc_found and not args.allow_missing_emcc:
        return 3
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

#!/usr/bin/env python3
"""Control the verified upstream QuickJS WASM cutover workflow.

This tool intentionally separates three states:

1. repository/toolchain readiness (`--preflight-only`),
2. verified artifact embedding (`--artifact quickjs-runtime.wasm`), and
3. strict embedded-real-engine verification (`--verify-embedded --require-real`).

A checked-in scaffold can still satisfy the ABI export shape, so this tool also
checks embedded provenance. `--require-real` only passes when the embedded header
claims a verified upstream QuickJS WASM artifact with all ABI12 exports present
and scaffold/contract markers disabled.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

PACKAGE_VERSION = 83
RUNTIME_ABI = 12

REQUIRED_REAL_PROVENANCE = {
    'runtime_abi': str(RUNTIME_ABI),
    'package_version': str(PACKAGE_VERSION),
    'artifact_kind': 'upstream-quickjs-wasm',
    'runtime_implementation': 'quickjs-wasm-upstream-quickjs',
    'runtime_claim': 'full-upstream-quickjs-wasm',
    'contract_only': 'false',
    'scaffold_runtime': 'false',
    'real_engine_candidate': 'true',
    'full_upstream_quickjs': 'true',
    'required_exports_satisfied': 'true',
    'missing_export_count': '0',
    'bytecode_format': 'VQJSBC03',
    'module_bundle_contract': 'VQJSMB04',
    'literal_dynamic_import': 'true',
    'native_object_reader': 'JS_ReadObject',
    'source_materialization': 'false',
    'venom_qjs_wasm_native_stack_capacity': '4194304',
}


def run(cmd: list[str], *, timeout: int = 90) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)


def parse_kv(text: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith('#') or '=' not in line:
            continue
        key, value = line.split('=', 1)
        out[key.strip()] = value.strip()
    return out


def embedded_provenance(header: Path) -> tuple[str, dict[str, str]]:
    if not header.exists():
        raise SystemExit(f'embedded WASM header not found: {header}')
    text = header.read_text(encoding='utf-8')
    m = re.search(r'kQuickJsRuntimeWasmBlobProvenance\s*=\s*R"[^\(]*\((.*?)\)[A-Za-z0-9_]*"', text, re.S)
    if not m:
        raise SystemExit(f'could not find kQuickJsRuntimeWasmBlobProvenance in {header}')
    provenance = m.group(1)
    return provenance, parse_kv(provenance)


def strict_real_failures(info: dict[str, str]) -> list[str]:
    failures: list[str] = []
    for key, expected in REQUIRED_REAL_PROVENANCE.items():
        actual = info.get(key, '')
        if actual != expected:
            failures.append(f'{key}: expected {expected!r}, got {actual!r}')
    blocker = info.get('finish_blocker', '')
    if blocker not in {'', 'none'}:
        failures.append(f'finish_blocker must be none, got {blocker!r}')
    return failures


def write_status(out_dir: Path, name: str, data: dict[str, object]) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / f'{name}.json').write_text(json.dumps(data, indent=2), encoding='utf-8')
    lines = [f'VENOM_QJS_WASM_CUTOVER_{name.upper()}_V1']
    for key, value in data.items():
        if isinstance(value, bool):
            value = str(value).lower()
        elif isinstance(value, list):
            value = ','.join(str(v) for v in value)
        lines.append(f'{key}={value}')
    (out_dir / f'{name}.txt').write_text('\n'.join(lines) + '\n', encoding='utf-8')


def preflight(repo_root: Path, out_dir: Path, emcc: str | None, allow_missing: bool) -> int:
    manifest = out_dir / 'quickjs-wasm-preflight.txt'
    report = out_dir / 'quickjs-wasm-preflight.json'
    cmd = [
        sys.executable,
        str(repo_root / 'tools' / 'quickjs_wasm_preflight.py'),
        '--repo-root', str(repo_root),
        '--manifest', str(manifest),
        '--json', str(report),
    ]
    if emcc:
        cmd.extend(['--emcc', emcc])
    if allow_missing:
        cmd.append('--allow-missing-emcc')
    result = run(cmd, timeout=45)
    print(result.stdout, end='')
    return result.returncode


def verify_and_embed_artifact(repo_root: Path, artifact: Path, out_dir: Path, embed_out: Path, skip_embed: bool) -> int:
    if not artifact.is_file():
        print(f'[venom] artifact not found: {artifact}', file=sys.stderr)
        return 2
    manifest = out_dir / 'quickjs-runtime.manifest.txt'
    exports_json = out_dir / 'quickjs-runtime.exports.json'
    release_abi_json = out_dir / 'quickjs-runtime.release-abi.json'
    abi_cmd = [sys.executable, str(repo_root / 'tools' / 'quickjs_release_abi.py'), str(release_abi_json)]
    abi_result = run(abi_cmd, timeout=30)
    print(abi_result.stdout, end='')
    if abi_result.returncode != 0:
        return abi_result.returncode
    inspect_cmd = [
        sys.executable,
        str(repo_root / 'tools' / 'wasm_exports.py'),
        str(artifact),
        '--require-json', str(release_abi_json),
        '--exact-exports',
        '--exports-json', str(exports_json),
        '--manifest', str(manifest),
        '--runtime-abi', str(RUNTIME_ABI),
        '--package-version', str(PACKAGE_VERSION),
        '--validate-runtime-values',
        '--release-runtime-values',
        '--fail-missing',
    ]
    inspected = run(inspect_cmd, timeout=90)
    print(inspected.stdout, end='')
    if inspected.returncode != 0:
        return inspected.returncode
    artifact_bytes = artifact.read_bytes()
    if b'VQJSBC03' not in artifact_bytes or b'VQJSBC02' in artifact_bytes:
        print('[venom] artifact is not the native VQJSBC03 runtime or still contains the legacy VQJSBC02 decoder', file=sys.stderr)
        return 3
    with manifest.open('a', encoding='utf-8') as fp:
        fp.write('bytecode_format=VQJSBC03\n')
        fp.write('module_bundle_contract=VQJSMB04\n')
        fp.write('literal_dynamic_import=true\n')
        fp.write('native_object_reader=JS_ReadObject\n')
        fp.write('source_materialization=false\n')
        fp.write('release_abi_exact=true\n')
        fp.write('release_export_count=23\n')
        fp.write('venom_qjs_wasm_native_stack_capacity=4194304\n')
    kv = parse_kv(manifest.read_text(encoding='utf-8'))
    if kv.get('artifact_kind') != 'upstream-quickjs-wasm' or kv.get('required_exports_satisfied') != 'true':
        print('[venom] artifact did not satisfy strict upstream QuickJS WASM manifest requirements', file=sys.stderr)
        return 3
    if not skip_embed:
        embed_cmd = [
            sys.executable,
            str(repo_root / 'tools' / 'embed_wasm.py'),
            str(artifact),
            '--manifest', str(manifest),
            '--out', str(embed_out),
        ]
        embedded = run(embed_cmd, timeout=90)
        print(embedded.stdout, end='')
        if embedded.returncode != 0:
            return embedded.returncode
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument('--out-dir', type=Path, default=None)
    ap.add_argument('--emcc', default=None)
    ap.add_argument('--allow-missing-emcc', action='store_true')
    ap.add_argument('--preflight-only', action='store_true')
    ap.add_argument('--artifact', type=Path, help='verified Emscripten-generated quickjs-runtime.wasm to inspect/embed')
    ap.add_argument('--embed-out', type=Path, default=None)
    ap.add_argument('--skip-embed', action='store_true')
    ap.add_argument('--verify-embedded', action='store_true')
    ap.add_argument('--require-real', action='store_true', help='fail unless embedded provenance proves real upstream QuickJS WASM')
    args = ap.parse_args()

    repo_root = args.repo_root.resolve()
    out_dir = (args.out_dir or (repo_root / 'build' / 'quickjs-wasm-cutover')).resolve()
    embed_out = (args.embed_out or (repo_root / 'src' / 'compiler' / 'quickjs_runtime_wasm_blob.hpp')).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    if args.preflight_only:
        return preflight(repo_root, out_dir, args.emcc, allow_missing=args.allow_missing_emcc)

    if args.artifact:
        # A real cutover should be performed on the machine that has the active
        # Emscripten toolchain. This prevents an ABI-shaped scaffold blob from
        # being reclassified as real only because its export names match.
        pf = preflight(repo_root, out_dir, args.emcc, allow_missing=False)
        if pf != 0:
            return pf
        rc = verify_and_embed_artifact(repo_root, args.artifact.resolve(), out_dir, embed_out, args.skip_embed)
        if rc != 0:
            return rc
        args.verify_embedded = not args.skip_embed
        args.require_real = not args.skip_embed

    if args.verify_embedded or args.require_real:
        provenance, info = embedded_provenance(embed_out)
        failures = strict_real_failures(info) if args.require_real else []
        status = {
            'package_version': info.get('package_version', ''),
            'runtime_abi': info.get('runtime_abi', ''),
            'artifact_kind': info.get('artifact_kind', ''),
            'runtime_implementation': info.get('runtime_implementation', ''),
            'real_engine_candidate': info.get('real_engine_candidate', ''),
            'full_upstream_quickjs': info.get('full_upstream_quickjs', ''),
            'required_exports_satisfied': info.get('required_exports_satisfied', ''),
            'finish_blocker': info.get('finish_blocker', ''),
            'strict_real': not failures,
            'failures': failures,
        }
        write_status(out_dir, 'embedded_status', status)
        print('[venom] embedded QuickJS WASM provenance:')
        for key in ('package_version', 'artifact_kind', 'runtime_implementation', 'real_engine_candidate', 'full_upstream_quickjs', 'required_exports_satisfied', 'finish_blocker'):
            print(f'[venom]   {key}: {info.get(key, "")}')
        if failures:
            print('[venom] strict real-engine verification failed:', file=sys.stderr)
            for failure in failures:
                print(f'[venom]   {failure}', file=sys.stderr)
            return 4
        print('[venom] strict real-engine verification: PASS' if args.require_real else '[venom] embedded provenance read: PASS')
        return 0

    print('[venom] nothing to do; pass --preflight-only, --artifact, or --verify-embedded')
    return 2


if __name__ == '__main__':
    raise SystemExit(main())

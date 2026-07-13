#!/usr/bin/env python3
"""Analyze a Venom hardened/browser dist folder.

This is a diagnostic tool: it does not modify the dist. It reports file hashes,
package/runtime truth metadata, production-only layout status, and QuickJS WASM
ABI/runtime-value status.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
import tempfile
import shutil
from pathlib import Path

PACKAGE_VERSION = 83
RUNTIME_ABI = 12


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def run(cmd: list[str], timeout: int = 90) -> tuple[int, str]:
    try:
        proc = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
        return proc.returncode, proc.stdout
    except subprocess.TimeoutExpired as exc:
        return 124, (exc.stdout or '') + '\n[venom] command timed out\n'


def find_one(patterns: list[str], root: Path) -> Path | None:
    matches: list[Path] = []
    for pattern in patterns:
        matches.extend(root.glob(pattern))
    return sorted(set(matches))[0] if matches else None


def parse_loader(loader: Path | None) -> dict[str, str]:
    if not loader or not loader.exists():
        return {}
    text = loader.read_text(encoding='utf-8', errors='replace')
    out: dict[str, str] = {}
    for key in ('packageUrl', 'runtimeUrl', 'runtimeWasmUrl', 'styleUrl', 'quickJsEngineUrl', 'quickJsWasmUrl', 'workerUrl', 'expectedPackageHash', 'expectedQuickJsWasmSha256', 'hardened'):
        m = re.search(rf'{key}\s*:\s*(?:new URL\(\'([^\']+)\'|\'([^\']+)\'|(true|false))', text)
        if m:
            out[key] = next((g for g in m.groups() if g is not None), '')
    return out


def parse_cli_report(output: str) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in output.splitlines():
        match = re.match(r"\s{2}([a-zA-Z0-9_]+):\s*(.*)$", line)
        if match:
            values[match.group(1)] = match.group(2).strip()
    return values


HASH_RE = r'[0-9a-f]{12}'
RESERVED_ASSET_ROOTS = {'app', 'style', 'loader', 'runtime', 'workers'}
FORBIDDEN_PRODUCTION_MARKERS = (
    'new Function', 'eval(', 'sourceMappingURL', 'host-js-fallback',
    'legacy-host-js-wrapper', 'strict_release=false', 'release_fail_closed=false',
    '__venomDisabledSourceEval', '__venomDisabledEval',
)


def validate_production_layout(dist: Path) -> list[str]:
    failures: list[str] = []
    rels = sorted(path.relative_to(dist).as_posix() for path in dist.rglob('*') if path.is_file())
    root_files = [rel for rel in rels if '/' not in rel]
    if root_files != ['index.html']:
        failures.append(f'root must contain only index.html, found: {root_files}')
    required_patterns = {
        'app package': rf'assets/app/app\.{HASH_RE}\.vbc',
        'stylesheet': rf'assets/style/s\.{HASH_RE}\.css',
        'loader': rf'assets/loader/loader\.{HASH_RE}\.js',
        'runtime bridge': rf'assets/runtime/r\.{HASH_RE}\.js',
        'QuickJS engine': rf'assets/runtime/engine\.{HASH_RE}\.js',
        'QuickJS WASM': rf'assets/runtime/runtime\.{HASH_RE}\.wasm',
        'runtime WASM': rf'assets/runtime/rw\.{HASH_RE}\.wasm',
        'worker': rf'assets/workers/worker\.{HASH_RE}\.js',
    }
    for label, pattern in required_patterns.items():
        if not any(re.fullmatch(pattern, rel) for rel in rels):
            failures.append(f'missing production {label}: {pattern}')
    forbidden_paths = {
        'sw.js', 'favicon.ico', 'assets/app.vbc', 'assets/asset-manifest.txt',
        'assets/runtime.js', 'assets/quickjs-engine.js', 'assets/quickjs-runtime.wasm',
    }
    for rel in rels:
        if rel in forbidden_paths or rel.endswith('.map') or rel.endswith('.d.ts'):
            failures.append(f'forbidden production artifact: {rel}')
        if rel.startswith('assets/'):
            parts = rel.split('/')
            if len(parts) >= 3 and parts[1] in RESERVED_ASSET_ROOTS:
                continue
            if len(parts) == 2 and parts[1] in {'app.vbc', 'loader.js', 'runtime.js', 'quickjs-engine.js'}:
                failures.append(f'legacy flat asset path is not allowed: {rel}')
    loader_files = [dist / rel for rel in rels if re.fullmatch(rf'assets/loader/loader\.{HASH_RE}\.js', rel)]
    if loader_files:
        loader_text = loader_files[0].read_text(encoding='utf-8', errors='replace')
        expected_loader_refs = [
            "assetBaseUrl: new URL('../', import.meta.url).href",
            "packageUrl: new URL('../app/app.",
            "styleUrl: new URL('../style/s.",
            "runtimeUrl: new URL('../runtime/r.",
            "quickJsEngineUrl: new URL('../runtime/engine.",
            "quickJsWasmUrl: new URL('../runtime/runtime.",
            "runtimeWasmUrl: new URL('../runtime/rw.",
            "workerUrl: new URL('../workers/worker.",
            "new Worker(bootOptions.workerUrl",
            "protocol: 1",
            "expectedQuickJsWasmSha256:",
        ]
        for needle in expected_loader_refs:
            if needle not in loader_text:
                failures.append(f'loader is missing grouped production reference: {needle}')
    runtime_files = [dist / rel for rel in rels if re.fullmatch(rf'assets/runtime/r\.{HASH_RE}\.js', rel)]
    if runtime_files:
        runtime_text = runtime_files[0].read_text(encoding='utf-8', errors='replace')
        if 'unvendored network script denied by production runtime' not in runtime_text:
            failures.append('runtime bridge is missing the fail-closed unvendored network script gate')

    index = dist / 'index.html'
    if index.exists():
        index_text = index.read_text(encoding='utf-8', errors='replace')
        if 'assets/loader/loader.' not in index_text or 'assets/style/s.' not in index_text:
            failures.append('index.html does not point at grouped loader/style assets')
        sri_values = re.findall(r'integrity="sha256-[A-Za-z0-9+/]+={0,2}"', index_text)
        if len(sri_values) != 2:
            failures.append(f'index.html must pin loader and stylesheet with SHA-256 SRI, found {len(sri_values)}')
    for rel in rels:
        path = dist / rel
        if path.suffix.lower() in {'.js', '.html', '.css'}:
            text = path.read_text(encoding='utf-8', errors='replace')
            for marker in FORBIDDEN_PRODUCTION_MARKERS:
                if marker in text:
                    failures.append(f'forbidden marker {marker!r} in {rel}')
    return failures


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('dist', type=Path)
    ap.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument('--venom', type=Path, default=None, help='optional venom executable for verify-runtime checks')
    ap.add_argument('--out', type=Path, default=None, help='write JSON report here')
    ap.add_argument('--manifest-dir', type=Path, default=None, help='directory for generated wasm manifests')
    ap.add_argument('--strict', action='store_true', help='return non-zero if strict real engine verification fails')
    args = ap.parse_args()

    dist = args.dist.resolve()
    repo = args.repo_root.resolve()
    if not dist.is_dir():
        print(f'[venom] dist folder not found: {dist}', file=sys.stderr)
        return 2

    files = []
    for path in sorted(p for p in dist.rglob('*') if p.is_file()):
        files.append({'path': str(path.relative_to(dist)).replace('\\', '/'), 'bytes': path.stat().st_size, 'sha256': sha256(path)})

    app = find_one(['assets/app/app.*.vbc'], dist)
    loader = find_one(['assets/loader/loader.*.js'], dist)
    qjs_wasm = find_one(['assets/runtime/runtime.*.wasm'], dist)
    runtime_wasm = find_one(['assets/runtime/rw.*.wasm'], dist)

    production_layout_failures = validate_production_layout(dist)
    recon_markers = {
        'semantic runtime names': ('executionJournal', 'capabilityLedger', 'snapshotCapture', 'determinismAudit'),
        'source evaluation': ('new Function', 'eval('),
        'debug metadata': ('sourceMappingURL', 'quickjs-probe', 'strict_release=false'),
    }
    marker_hits: dict[str, list[str]] = {}
    text_assets = []
    for path in sorted(p for p in dist.rglob('*') if p.is_file() and p.suffix.lower() in {'.js', '.html', '.css'}):
        text = path.read_text(encoding='utf-8', errors='replace')
        rel = path.relative_to(dist).as_posix()
        for category, markers in recon_markers.items():
            for marker in markers:
                if marker in text:
                    marker_hits.setdefault(category, []).append(f'{rel}:{marker}')
        text_assets.append((rel, text))
    reconnaissance_score = max(0, 100 - sum(12 * len(v) for v in marker_hits.values()))

    report: dict[str, object] = {
        'dist': str(dist),
        'file_count': len(files),
        'files': files,
        'app_package': str(app.relative_to(dist)) if app else '',
        'loader': str(loader.relative_to(dist)) if loader else '',
        'quickjs_wasm': str(qjs_wasm.relative_to(dist)) if qjs_wasm else '',
        'runtime_wasm': str(runtime_wasm.relative_to(dist)) if runtime_wasm else '',
        'loader_config': parse_loader(loader),
        'production_layout_pass': not production_layout_failures,
        'production_layout_failures': production_layout_failures,
        'reconnaissance_score': reconnaissance_score,
        'reconnaissance_marker_hits': marker_hits,
    }

    if args.venom:
        rc, out = run([str(args.venom), 'verify-runtime', str(dist), '--target', 'browser'])
        report['verify_runtime_returncode'] = rc
        report['verify_runtime_output'] = out
        report['verify_runtime_values'] = parse_cli_report(out)
        rc2, out2 = run([str(args.venom), 'verify-runtime', str(dist), '--target', 'browser', '--require-real-engine'])
        report['verify_runtime_require_real_returncode'] = rc2
        report['verify_runtime_require_real_output'] = out2
        report['verify_runtime_require_real_values'] = parse_cli_report(out2)
        real_values = report['verify_runtime_require_real_values']
        report['remote_vendor_metadata'] = real_values.get('remote_vendor_metadata', 'no') == 'yes'
        report['remote_vendor_count'] = int(real_values.get('remote_vendor_count', '0') or 0)
        report['remote_vendor_lock'] = real_values.get('remote_vendor_lock', 'no') == 'yes'
        report['remote_vendor_lock_mode'] = real_values.get('remote_vendor_lock_mode', '')
        report['remote_vendor_lock_entries'] = int(real_values.get('remote_vendor_lock_entries', '0') or 0)
        report['remote_vendor_lock_sha256'] = real_values.get('remote_vendor_lock_sha256', '')
        report['vendored_remote_chunks'] = int(real_values.get('vendored_remote_chunks', '0') or 0)
        report['runtime_remote_chunks'] = int(real_values.get('runtime_remote_chunks', '0') or 0)

    temporary_manifest_dir: Path | None = None
    if qjs_wasm:
        if args.manifest_dir:
            manifest_dir = args.manifest_dir.resolve()
            manifest_dir.mkdir(parents=True, exist_ok=True)
        else:
            temporary_manifest_dir = Path(tempfile.mkdtemp(prefix='venom-analysis-')).resolve()
            manifest_dir = temporary_manifest_dir
        manifest = manifest_dir / 'quickjs-runtime.analysis.manifest.txt'
        exports_json = manifest_dir / 'quickjs-runtime.analysis.exports.json'
        release_abi_json = manifest_dir / 'quickjs-runtime.release-abi.json'
        abi_rc, abi_out = run([
            sys.executable,
            str(repo / 'tools' / 'quickjs_release_abi.py'),
            str(release_abi_json),
        ])
        if abi_rc != 0:
            rc, out = abi_rc, abi_out
        else:
            cmd = [
                sys.executable,
                str(repo / 'tools' / 'wasm_exports.py'),
                str(qjs_wasm),
                '--require-json', str(release_abi_json),
                '--exact-exports',
                '--manifest', str(manifest),
                '--exports-json', str(exports_json),
                '--runtime-abi', str(RUNTIME_ABI),
                '--package-version', str(PACKAGE_VERSION),
                '--validate-runtime-values',
                '--release-runtime-values',
                '--fail-missing',
            ]
            rc, out = run(cmd)
        report['quickjs_wasm_value_returncode'] = rc
        report['quickjs_wasm_value_output'] = out
        report['quickjs_wasm_analysis_manifest'] = str(manifest)
        report['quickjs_wasm_analysis_exports_json'] = str(exports_json)
        if manifest.exists():
            report['quickjs_wasm_analysis_manifest_text'] = manifest.read_text(encoding='utf-8')

    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(json.dumps(report, indent=2), encoding='utf-8')

    if temporary_manifest_dir is not None:
        shutil.rmtree(temporary_manifest_dir, ignore_errors=True)

    print('[venom] dist analysis')
    print(f'[venom]   files: {report["file_count"]}')
    print(f'[venom]   app: {report["app_package"] or "missing"}')
    print(f'[venom]   production layout: {"PASS" if report.get("production_layout_pass") else "FAIL"}')
    print(f'[venom]   quickjs wasm: {report["quickjs_wasm"] or "missing"}')
    print(f'[venom]   reconnaissance score: {report["reconnaissance_score"]}/100')
    if 'verify_runtime_returncode' in report:
        print(f'[venom]   verify-runtime: rc={report["verify_runtime_returncode"]}')
        print(f'[venom]   verify-runtime --require-real-engine: rc={report["verify_runtime_require_real_returncode"]}')
    if 'remote_vendor_metadata' in report:
        print(f'[venom]   remote vendors: {report["remote_vendor_count"]} packaged, {report["runtime_remote_chunks"]} runtime URL chunks')
        print(f'[venom]   vendor lock: {"PASS" if report.get("remote_vendor_lock") else "FAIL"} mode={report.get("remote_vendor_lock_mode") or "none"} entries={report.get("remote_vendor_lock_entries", 0)}')
    if 'quickjs_wasm_value_returncode' in report:
        print(f'[venom]   quickjs wasm runtime value gate: rc={report["quickjs_wasm_value_returncode"]}')
    if args.out:
        print(f'[venom]   report: {args.out}')

    if args.strict:
        if not report.get('production_layout_pass'):
            for failure in production_layout_failures:
                print(f'[venom]   layout failure: {failure}', file=sys.stderr)
            return 1
        if report.get('verify_runtime_require_real_returncode', 0) != 0 or report.get('quickjs_wasm_value_returncode', 0) != 0:
            return 1
        if 'remote_vendor_lock' in report and not report.get('remote_vendor_lock'):
            return 1
        if int(report.get('reconnaissance_score', 0)) < 88:
            print('[venom]   reconnaissance score below strict release threshold (88)', file=sys.stderr)
            return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

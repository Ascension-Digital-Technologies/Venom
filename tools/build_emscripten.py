#!/usr/bin/env python3
"""Bootstrap Emscripten and run Venom's verified QuickJS WASM cutover build.

This controller is intentionally conservative:
- `--preflight-only` never downloads anything and may pass with `--allow-missing`.
- the default mode installs/activates emsdk when needed, then runs build-quickjs-wasm
  inside the activated SDK environment.
- a checked-in scaffold is never reclassified as a real QuickJS engine; the normal
  cutover gates still inspect/export/embed/verify the generated .wasm artifact.
"""
from __future__ import annotations

import argparse
import json
import os
import platform
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

PACKAGE_VERSION = 83


def is_windows() -> bool:
    return os.name == 'nt'


def find_exe(name: str | None) -> str:
    if not name:
        return ''
    found = shutil.which(name)
    if found:
        return found
    p = Path(name)
    return str(p) if p.exists() else ''


def run(cmd: list[str], *, cwd: Path | None = None, timeout: int = 3600, env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    print('[venom] ' + ' '.join(cmd))
    return subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout,
        env=env,
    )


def run_stream(cmd: list[str], *, cwd: Path | None = None, timeout: int = 7200) -> int:
    print('[venom] ' + ' '.join(cmd))
    try:
        proc = subprocess.run(cmd, cwd=str(cwd) if cwd else None, timeout=timeout)
    except subprocess.TimeoutExpired:
        print('[venom] command timed out', file=sys.stderr)
        return 124
    return proc.returncode


def kv_lines(title: str, info: dict[str, object]) -> str:
    lines = [title]
    for key, value in info.items():
        if isinstance(value, bool):
            value = str(value).lower()
        elif isinstance(value, list):
            value = ','.join(str(v) for v in value)
        lines.append(f'{key}={value}')
    return '\n'.join(lines) + '\n'


def write_status(out_dir: Path, status: dict[str, object]) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / 'emscripten-build.json').write_text(json.dumps(status, indent=2), encoding='utf-8')
    (out_dir / 'emscripten-build.txt').write_text(kv_lines('VENOM_EMSCRIPTEN_BUILD_V1', status), encoding='utf-8')


def setup_report(repo_root: Path, out_dir: Path, emsdk_dir: Path, emcc: str, allow_missing: bool, check_only: bool, skip_download: bool, version: str) -> tuple[int, dict[str, object]]:
    setup_out = out_dir / 'setup'
    cmd = [
        sys.executable,
        str(repo_root / 'tools' / 'setup_emscripten.py'),
        '--repo-root', str(repo_root),
        '--emsdk-dir', str(emsdk_dir),
        '--out-dir', str(setup_out),
        '--version', version,
        '--emcc', emcc,
    ]
    if check_only:
        cmd.append('--check-only')
    if allow_missing:
        cmd.append('--allow-missing')
    if skip_download:
        cmd.append('--skip-download')
    result = run(cmd, timeout=2400)
    print(result.stdout, end='')
    report_path = setup_out / 'emscripten-setup.json'
    report: dict[str, object] = {}
    if report_path.exists():
        report = json.loads(report_path.read_text(encoding='utf-8'))
    return result.returncode, report


def preflight_report(repo_root: Path, out_dir: Path, emcc: str, allow_missing: bool) -> tuple[int, dict[str, object]]:
    pf_out = out_dir / 'preflight'
    pf_out.mkdir(parents=True, exist_ok=True)
    cmd = [
        sys.executable,
        str(repo_root / 'tools' / 'quickjs_wasm_preflight.py'),
        '--repo-root', str(repo_root),
        '--emcc', emcc,
        '--manifest', str(pf_out / 'quickjs-wasm-preflight.txt'),
        '--json', str(pf_out / 'quickjs-wasm-preflight.json'),
    ]
    if allow_missing:
        cmd.append('--allow-missing-emcc')
    result = run(cmd, timeout=90)
    print(result.stdout, end='')
    report_path = pf_out / 'quickjs-wasm-preflight.json'
    report: dict[str, object] = {}
    if report_path.exists():
        report = json.loads(report_path.read_text(encoding='utf-8'))
    return result.returncode, report


def shell_build_command(repo_root: Path, emsdk_dir: Path, out_dir: Path, emcc: str) -> list[str]:
    build_script = repo_root / 'scripts' / ('build-quickjs-wasm.bat' if is_windows() else 'build-quickjs-wasm.sh')
    if is_windows():
        # Do not chain through `cmd /c call emsdk_env.bat && ...` here.
        # On Windows, that command is fragile when paths contain spaces/quotes and
        # it already caused cmd.exe to try to execute the quote characters
        # literally. setup_emscripten.py resolves the installed emcc.exe/emcc.bat
        # path for us, and EMCC is propagated below, so the QuickJS build can run
        # directly without needing parent-shell PATH mutation.
        ps_script = repo_root / 'scripts' / 'build-quickjs-wasm.ps1'
        if shutil.which('powershell.exe') or shutil.which('pwsh.exe'):
            shell = shutil.which('powershell.exe') or shutil.which('pwsh.exe') or 'powershell.exe'
            return [shell, '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', str(ps_script), '-OutDir', str(out_dir), '-Emcc', emcc]
        return [str(build_script), '-OutDir', str(out_dir), '-Emcc', emcc]
    env_sh = emsdk_dir / 'emsdk_env.sh'
    if env_sh.exists() and not find_exe(emcc):
        command = f'source "{env_sh}" >/dev/null && "{build_script}" "{out_dir}"'
        return ['bash', '-lc', command]
    return [str(build_script), str(out_dir)]


def verify_embedded(repo_root: Path, out_dir: Path) -> int:
    cmd = [
        sys.executable,
        str(repo_root / 'tools' / 'quickjs_wasm_cutover.py'),
        '--repo-root', str(repo_root),
        '--out-dir', str(out_dir / 'embedded-verify'),
        '--verify-embedded',
        '--require-real',
    ]
    result = run(cmd, timeout=90)
    print(result.stdout, end='')
    return result.returncode

def rebuild_native_compiler(repo_root: Path) -> tuple[int, Path | None]:
    cmake = find_exe('cmake')
    if not cmake:
        print('[venom] cmake was not found; cannot rebuild the native compiler after embedding QuickJS WASM', file=sys.stderr)
        return 127, None
    build_dir = repo_root / 'build' / ('windows-release' if is_windows() else 'native-release')
    configure = [
        cmake,
        '-S', str(repo_root),
        '-B', str(build_dir),
        '-DCMAKE_BUILD_TYPE=Release',
        '-DVENOM_ENABLE_FULL_QUICKJS=ON',
    ]
    rc = run_stream(configure, cwd=repo_root, timeout=1800)
    if rc != 0:
        return rc, None
    rc = run_stream([cmake, '--build', str(build_dir), '--config', 'Release', '--parallel'], cwd=repo_root, timeout=7200)
    if rc != 0:
        return rc, None
    names = ['venom.exe', 'venom'] if is_windows() else ['venom', 'venom.exe']
    candidates: list[Path] = []
    for name in names:
        candidates.extend([build_dir / name, build_dir / 'Release' / name])
    for candidate in candidates:
        if candidate.exists():
            print(f'[venom] rebuilt native compiler with the new embedded QuickJS WASM: {candidate}')
            return 0, candidate
    print(f'[venom] native build completed but venom executable was not found under {build_dir}', file=sys.stderr)
    return 1, None


def verify_native_runtime_binding(repo_root: Path, out_dir: Path, venom: Path) -> int:
    smoke_dist = out_dir / 'runtime-binding-smoke-dist'
    if smoke_dist.exists():
        shutil.rmtree(smoke_dist)
    build = run_stream([str(venom), 'build', str(repo_root / 'examples' / 'basic-site'), '--out', str(smoke_dist)], cwd=repo_root, timeout=1200)
    if build != 0:
        return build
    verify = run_stream([str(venom), 'verify-runtime', str(smoke_dist), '--target', 'browser', '--require-real-engine'], cwd=repo_root, timeout=300)
    if verify == 0:
        print('[venom] native compiler/runtime binding smoke passed')
    return verify


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument('--out-dir', type=Path, default=None, help='status/report output directory')
    ap.add_argument('--wasm-out-dir', type=Path, default=None, help='QuickJS WASM build output directory')
    ap.add_argument('--emsdk-dir', type=Path, default=None)
    ap.add_argument('--version', default=os.environ.get('EMSDK_VERSION', 'latest'), help='emsdk version to install/activate')
    ap.add_argument('--emcc', default=os.environ.get('EMCC', 'emcc'))
    ap.add_argument('--preflight-only', action='store_true')
    ap.add_argument('--allow-missing', action='store_true', help='allow smoke/preflight success even when emcc is not installed')
    ap.add_argument('--skip-download', action='store_true', help='do not clone emsdk; require existing checkout/toolchain')
    ap.add_argument('--skip-setup', action='store_true', help='do not run setup_emscripten before build')
    ap.add_argument('--skip-build', action='store_true', help='only perform setup/preflight, do not build QuickJS WASM')
    ap.add_argument('--skip-final-verify', action='store_true', help='skip strict embedded real-engine verification after build')
    ap.add_argument('--skip-native-rebuild', action='store_true', help='do not rebuild the native compiler after the embedded WASM header changes')
    ap.add_argument('--skip-runtime-smoke', action='store_true', help='skip the native compiler/runtime binding smoke distribution')
    args = ap.parse_args()

    repo_root = args.repo_root.resolve()
    out_dir = (args.out_dir or (repo_root / 'build' / 'emscripten-build')).resolve()
    wasm_out = (args.wasm_out_dir or (repo_root / 'build' / 'quickjs-wasm')).resolve()
    emsdk_dir = (args.emsdk_dir or (repo_root / 'build' / 'emsdk')).resolve()
    emcc = args.emcc

    status: dict[str, object] = {
        'package_version': PACKAGE_VERSION,
        'status': 'started',
        'timestamp_utc': datetime.now(timezone.utc).isoformat(),
        'repo_root': str(repo_root),
        'host_platform': platform.platform(),
        'python': sys.executable,
        'emsdk_dir': str(emsdk_dir),
        'emsdk_version': args.version,
        'emcc_requested': emcc,
        'emcc_initial': find_exe(emcc),
        'preflight_only': args.preflight_only,
        'skip_download': args.skip_download,
        'skip_setup': args.skip_setup,
        'skip_build': args.skip_build,
        'skip_native_rebuild': args.skip_native_rebuild,
        'skip_runtime_smoke': args.skip_runtime_smoke,
    }
    write_status(out_dir, status)

    if args.preflight_only:
        setup_rc, setup = setup_report(repo_root, out_dir, emsdk_dir, emcc, allow_missing=args.allow_missing, check_only=True, skip_download=True, version=args.version)
        preflight_rc, preflight = preflight_report(repo_root, out_dir, emcc, allow_missing=args.allow_missing)
        status.update({
            'setup_returncode': setup_rc,
            'preflight_returncode': preflight_rc,
            'setup_status': setup.get('status', ''),
            'preflight_status': preflight.get('status', ''),
            'repo_inputs_ok': preflight.get('repo_inputs_ok', False),
            'emcc_found': bool(preflight.get('emcc_found', False)),
            'cutover_ready': bool(preflight.get('cutover_ready', False)),
            'required_export_count': len(preflight.get('exports', [])),
            'missing_required_export_count': len(preflight.get('missing_required_exports', [])),
            'status': 'preflight-ok' if setup_rc == 0 and preflight_rc == 0 else 'preflight-failed',
        })
        write_status(out_dir, status)
        return 0 if (setup_rc == 0 and preflight_rc == 0) else 1

    setup: dict[str, object] = {}
    if not args.skip_setup:
        setup_rc, setup = setup_report(repo_root, out_dir, emsdk_dir, emcc, allow_missing=False, check_only=False, skip_download=args.skip_download, version=args.version)
        resolved_emcc = str(setup.get('emcc_path') or '')
        if resolved_emcc:
            # Important on Windows: emsdk.bat activation happens in a child cmd.exe
            # and cannot update this Python process environment. Use the installed
            # emcc.bat path reported by setup_emscripten.py for preflight and
            # propagate it to child build scripts via EMCC.
            emcc = resolved_emcc
            os.environ['EMCC'] = resolved_emcc
            status['emcc_resolved_after_setup'] = resolved_emcc
        status.update({'setup_returncode': setup_rc, 'setup_status': setup.get('status', '')})
        write_status(out_dir, status)
        if setup_rc != 0:
            status['status'] = 'setup-failed'
            write_status(out_dir, status)
            return setup_rc

    preflight_rc, preflight = preflight_report(repo_root, out_dir, emcc, allow_missing=False)
    status.update({
        'preflight_returncode': preflight_rc,
        'preflight_status': preflight.get('status', ''),
        'repo_inputs_ok': preflight.get('repo_inputs_ok', False),
        'emcc_found': bool(preflight.get('emcc_found', False)),
        'cutover_ready': bool(preflight.get('cutover_ready', False)),
        'required_export_count': len(preflight.get('exports', [])),
        'missing_required_export_count': len(preflight.get('missing_required_exports', [])),
    })
    write_status(out_dir, status)
    if preflight_rc != 0:
        status['status'] = 'preflight-failed'
        write_status(out_dir, status)
        return preflight_rc
    if args.skip_build:
        status['status'] = 'ready-no-build'
        write_status(out_dir, status)
        return 0

    build_cmd = shell_build_command(repo_root, emsdk_dir, wasm_out, emcc)
    build_rc = run_stream(build_cmd, cwd=repo_root, timeout=7200)
    status['build_returncode'] = build_rc
    status['wasm_out_dir'] = str(wasm_out)
    status['wasm_artifact'] = str(wasm_out / 'quickjs-runtime.wasm')
    write_status(out_dir, status)
    if build_rc != 0:
        status['status'] = 'build-failed'
        write_status(out_dir, status)
        return build_rc

    verify_rc = 0 if args.skip_final_verify else verify_embedded(repo_root, out_dir)
    status['verify_returncode'] = verify_rc
    if verify_rc != 0:
        status['status'] = 'verify-failed'
        write_status(out_dir, status)
        return verify_rc

    native_rc = 0
    venom_exe: Path | None = None
    if not args.skip_native_rebuild:
        native_rc, venom_exe = rebuild_native_compiler(repo_root)
    status['native_rebuild_returncode'] = native_rc
    status['native_executable'] = str(venom_exe) if venom_exe else ''
    if native_rc != 0:
        status['status'] = 'native-rebuild-failed'
        write_status(out_dir, status)
        return native_rc

    smoke_rc = 0
    if not args.skip_runtime_smoke and venom_exe is not None:
        smoke_rc = verify_native_runtime_binding(repo_root, out_dir, venom_exe)
    status['runtime_binding_smoke_returncode'] = smoke_rc
    status['status'] = 'complete' if smoke_rc == 0 else 'runtime-binding-smoke-failed'
    write_status(out_dir, status)
    return smoke_rc


if __name__ == '__main__':
    raise SystemExit(main())

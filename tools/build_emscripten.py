#!/usr/bin/env python3
"""Canonical Emscripten controller for Venom's verified QuickJS/WASM runtime."""
from __future__ import annotations
import argparse, json, os, shutil, subprocess, sys
from pathlib import Path

PACKAGE_VERSION = 83


def is_windows() -> bool:
    return os.name == 'nt'


def run(cmd: list[str], *, cwd: Path | None = None, timeout: int = 3600) -> subprocess.CompletedProcess[str]:
    print('[venom] ' + ' '.join(str(x) for x in cmd))
    return subprocess.run(cmd, cwd=str(cwd) if cwd else None, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)


def shell_build_command(repo: Path, emsdk_dir: Path, out_dir: Path, emcc: str) -> list[str]:
    if is_windows():
        powershell = shutil.which('powershell.exe') or shutil.which('pwsh.exe') or 'powershell.exe'
        return [powershell, '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                str(repo / 'tools' / 'windows-scripts' / 'build-quickjs-wasm.ps1'),
                '-ControllerBuild', '-OutDir', str(out_dir), '-Emcc', emcc, '-Force']
    return ['bash', str(repo / 'tools' / 'linux-scripts' / 'build-quickjs-wasm.sh'),
            '--controller-build', '--out-dir', str(out_dir), '--emcc', emcc, '--force']


def write_report(out_dir: Path, info: dict[str, object]) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / 'emscripten-build.json').write_text(json.dumps(info, indent=2) + '\n', encoding='utf-8')
    lines = ['VENOM_EMSCRIPTEN_BUILD_V1']
    for key, value in info.items():
        if isinstance(value, bool): value = str(value).lower()
        lines.append(f'{key}={value}')
    (out_dir / 'emscripten-build.txt').write_text('\n'.join(lines) + '\n', encoding='utf-8')


def load_json(path: Path) -> dict:
    try: return json.loads(path.read_text(encoding='utf-8'))
    except Exception: return {}


def rebuild_native_compiler(repo: Path, build_dir: Path) -> int:
    configure = run(['cmake', '-S', str(repo), '-B', str(build_dir), '-DCMAKE_BUILD_TYPE=Release'], timeout=600)
    print(configure.stdout, end='')
    if configure.returncode: return configure.returncode
    built = run(['cmake', '--build', str(build_dir), '--config', 'Release', '--parallel', '2'], timeout=3600)
    print(built.stdout, end='')
    return built.returncode


def find_venom(build_dir: Path) -> Path | None:
    names = ['venom.exe', 'venom']
    for name in names:
        for path in (build_dir / name, build_dir / 'Release' / name, build_dir / 'bin' / name, build_dir / 'bin' / 'Release' / name):
            if path.is_file(): return path
    for name in names:
        matches = list(build_dir.rglob(name))
        if matches: return matches[0]
    return None


def verify_native_runtime_binding(repo: Path, build_dir: Path) -> int:
    venom = find_venom(build_dir)
    if not venom:
        print('[venom] rebuilt native compiler was not found', file=sys.stderr); return 4
    example = repo / 'examples' / 'protected-chess'
    if not example.is_dir():
        print('[venom] protected runtime smoke fixture is unavailable; skipping distribution smoke')
        return 0
    dist = build_dir / 'quickjs-runtime-smoke-dist'
    built = run([str(venom), 'build', str(example), '--out', str(dist), '--profile', 'prod'], timeout=900)
    print(built.stdout, end='')
    if built.returncode: return built.returncode
    verified = run([str(venom), 'verify-runtime', str(dist), '--require-real-engine'], timeout=300)
    print(verified.stdout, end='')
    return verified.returncode


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument('--out-dir', type=Path, default=None)
    ap.add_argument('--emsdk-dir', type=Path, default=None)
    ap.add_argument('--emcc', default='')
    ap.add_argument('--version', default='4.0.10')
    ap.add_argument('--preflight-only', action='store_true')
    ap.add_argument('--allow-missing', action='store_true')
    ap.add_argument('--skip-download', action='store_true')
    ap.add_argument('--skip-setup', action='store_true')
    ap.add_argument('--clean', action='store_true')
    ap.add_argument('--force', action='store_true')
    ap.add_argument('--skip-native-rebuild', action='store_true')
    ap.add_argument('--skip-runtime-smoke', action='store_true')
    args = ap.parse_args()

    repo = args.repo_root.resolve()
    out = (args.out_dir or repo / 'build' / 'emscripten-build').resolve()
    emsdk = (args.emsdk_dir or repo / 'build' / 'emsdk').resolve()
    if args.clean and out.exists(): shutil.rmtree(out)
    out.mkdir(parents=True, exist_ok=True)

    setup_out = out / 'setup'
    setup_cmd = [sys.executable, str(repo / 'tools' / 'setup_emscripten.py'), '--repo-root', str(repo),
                 '--emsdk-dir', str(emsdk), '--out-dir', str(setup_out), '--version', args.version]
    if args.emcc: setup_cmd += ['--emcc', args.emcc]
    if args.preflight_only or args.skip_setup: setup_cmd += ['--check-only']
    if args.allow_missing: setup_cmd += ['--allow-missing']
    if args.skip_download: setup_cmd += ['--skip-download']
    setup = run(setup_cmd, timeout=2400); print(setup.stdout, end='')
    setup_info = load_json(setup_out / 'emscripten-setup.json')
    emcc = str(setup_info.get('emcc_path') or args.emcc or '')

    preflight_out = out / 'preflight'
    preflight_cmd = [sys.executable, str(repo / 'tools' / 'quickjs_wasm_cutover.py'),
                     '--repo-root', str(repo), '--out-dir', str(preflight_out), '--preflight-only']
    if emcc: preflight_cmd += ['--emcc', emcc]
    if args.allow_missing: preflight_cmd += ['--allow-missing-emcc']
    preflight = run(preflight_cmd, timeout=90); print(preflight.stdout, end='')
    pf = load_json(preflight_out / 'quickjs-wasm-preflight.json')

    status = 'preflight-ok' if setup.returncode == 0 and preflight.returncode == 0 else 'preflight-failed'
    info = {
        'package_version': PACKAGE_VERSION, 'status': status,
        'repo_inputs_ok': bool(pf.get('repo_inputs_ok')),
        'emcc_found': bool(emcc), 'emcc_path': emcc,
        'required_export_count': len(pf.get('exports', [])),
        'missing_required_export_count': len(pf.get('missing_exports', [])),
        'runtime_built': False, 'native_rebuilt': False, 'runtime_smoke_passed': False,
    }
    write_report(out, info)
    if setup.returncode != 0 or preflight.returncode != 0: return setup.returncode or preflight.returncode
    if args.preflight_only: return 0
    if not emcc:
        print('[venom] emcc is required for a full runtime build', file=sys.stderr); return 3

    # Accept either a controller workspace or the runtime output directory itself.
    # This also prevents accidental recursive `quickjs-wasm/quickjs-wasm/...` growth.
    wasm_out = out if out.name.lower() == 'quickjs-wasm' else out / 'quickjs-wasm'
    build = run(shell_build_command(repo, emsdk, wasm_out, emcc), timeout=3600); print(build.stdout, end='')
    if build.returncode: return build.returncode
    info['runtime_built'] = True

    native_dir = out / 'native'
    if not args.skip_native_rebuild:
        rc = rebuild_native_compiler(repo, native_dir)
        if rc: return rc
        info['native_rebuilt'] = True
        if not args.skip_runtime_smoke:
            rc = verify_native_runtime_binding(repo, native_dir)
            if rc: return rc
            info['runtime_smoke_passed'] = True
    info['status'] = 'ready'
    write_report(out, info)
    return 0

if __name__ == '__main__':
    raise SystemExit(main())

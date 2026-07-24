#!/usr/bin/env python3
"""Bootstrap or inspect the Emscripten SDK used for Venom's TurboJS WASM build."""
from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

PACKAGE_VERSION = 83
EMSDK_URL = 'https://github.com/emscripten-core/emsdk.git'


def which(name: str | None) -> str:
    if not name:
        return ''
    return shutil.which(name) or (name if Path(name).exists() else '')


def emsdk_emcc_candidates(emsdk_dir: Path) -> list[Path]:
    """Return likely emcc entrypoints inside a local emsdk checkout.

    `emsdk.bat activate` changes the environment of the spawned cmd process, not
    this Python process. On Windows that means a just-installed emsdk can still
    look like `missing-emcc` unless we explicitly resolve the installed
    upstream/emscripten/emcc.bat path.
    """
    if os.name == 'nt':
        return [
            emsdk_dir / 'upstream' / 'emscripten' / 'emcc.bat',
            emsdk_dir / 'upstream' / 'emscripten' / 'emcc.cmd',
            emsdk_dir / 'upstream' / 'emscripten' / 'emcc.exe',
            emsdk_dir / 'upstream' / 'emscripten' / 'emcc.py',
        ]
    return [
        emsdk_dir / 'upstream' / 'emscripten' / 'emcc',
        emsdk_dir / 'upstream' / 'emscripten' / 'emcc.py',
    ]


def resolve_emcc(requested: str | None, emsdk_dir: Path) -> str:
    found = which(requested)
    if found:
        return found
    for candidate in emsdk_emcc_candidates(emsdk_dir):
        if candidate.exists():
            return str(candidate)
    return ''


def command_for_tool(exe: str, args: list[str] | None = None) -> list[str]:
    args = args or []
    # Running .bat/.cmd directly from Python is inconsistent across Windows
    # versions. Route it through cmd.exe so --version and preflight checks work.
    if os.name == 'nt' and exe.lower().endswith(('.bat', '.cmd')):
        return ['cmd.exe', '/d', '/s', '/c', subprocess.list2cmdline([exe, *args])]
    return [exe, *args]


def run(cmd: list[str], cwd: Path | None = None, timeout: int = 600) -> subprocess.CompletedProcess[str]:
    print('[venom] ' + ' '.join(cmd))
    return subprocess.run(cmd, cwd=str(cwd) if cwd else None, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)


def first_line(cmd: list[str], timeout: int = 20) -> str:
    try:
        p = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
    except Exception as exc:
        return f'unavailable: {exc}'
    return (p.stdout.splitlines()[0] if p.stdout else f'exit={p.returncode}').strip()


def tool_first_line(exe: str, args: list[str], timeout: int = 20) -> str:
    return first_line(command_for_tool(exe, args), timeout=timeout)


def write_reports(out_dir: Path, info: dict[str, object]) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / 'emscripten-setup.json').write_text(json.dumps(info, indent=2), encoding='utf-8')
    lines = ['VENOM_EMSCRIPTEN_SETUP_PREFLIGHT_V1']
    for key, value in info.items():
        if isinstance(value, bool):
            value = str(value).lower()
        lines.append(f'{key}={value}')
    (out_dir / 'emscripten-setup.txt').write_text('\n'.join(lines) + '\n', encoding='utf-8')


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument('--emsdk-dir', type=Path, default=None)
    ap.add_argument('--out-dir', type=Path, default=None)
    ap.add_argument('--version', default='4.0.10', help='emsdk version to install/activate; default 4.0.10 (verified release toolchain)')
    ap.add_argument('--emcc', default=os.environ.get('EMCC', 'emcc'))
    ap.add_argument('--check-only', action='store_true')
    ap.add_argument('--allow-missing', action='store_true', help='return success from --check-only even when emcc is missing')
    ap.add_argument('--skip-download', action='store_true', help='require an existing emsdk checkout instead of cloning')
    args = ap.parse_args()

    repo = args.repo_root.resolve()
    emsdk_dir = (args.emsdk_dir or (repo / 'build' / 'emsdk')).resolve()
    out_dir = (args.out_dir or (repo / 'build' / 'emscripten-setup')).resolve()
    emcc_path = resolve_emcc(args.emcc, emsdk_dir)
    emsdk_script = emsdk_dir / ('emsdk.bat' if os.name == 'nt' else 'emsdk')
    git_path = which('git')
    python_path = sys.executable

    if not args.check_only and not emcc_path:
        if not emsdk_dir.exists():
            if args.skip_download:
                print(f'[venom] emsdk checkout missing: {emsdk_dir}', file=sys.stderr)
            else:
                if not git_path:
                    print('[venom] git is required to clone emsdk', file=sys.stderr)
                    write_reports(out_dir, {
                        'package_version': PACKAGE_VERSION,
                        'status': 'missing-git',
                        'emcc_found': False,
                        'emsdk_dir': str(emsdk_dir),
                        'git_found': False,
                    })
                    return 2
                res = run([git_path, 'clone', EMSDK_URL, str(emsdk_dir)], timeout=900)
                print(res.stdout, end='')
                if res.returncode != 0:
                    return res.returncode
        if not emsdk_script.exists():
            print(f'[venom] emsdk script not found after setup: {emsdk_script}', file=sys.stderr)
            return 3
        install = run([str(emsdk_script), 'install', args.version], cwd=emsdk_dir, timeout=1800)
        print(install.stdout, end='')
        if install.returncode != 0:
            return install.returncode
        activate = run([str(emsdk_script), 'activate', args.version], cwd=emsdk_dir, timeout=600)
        print(activate.stdout, end='')
        if activate.returncode != 0:
            return activate.returncode
        # Resolve again after activation. `emsdk.bat activate` cannot mutate this
        # already-running Python process, so also check the installed emsdk path.
        emcc_path = resolve_emcc(args.emcc, emsdk_dir)

    status = 'ready' if emcc_path else 'missing-emcc'
    info = {
        'package_version': PACKAGE_VERSION,
        'status': status,
        'repo_root': str(repo),
        'emsdk_dir': str(emsdk_dir),
        'emcc_found': bool(emcc_path),
        'emcc_path': emcc_path,
        'emcc_version': tool_first_line(emcc_path, ['--version']) if emcc_path else '',
        'emsdk_script_found': emsdk_script.exists(),
        'emsdk_script': str(emsdk_script),
        'git_found': bool(git_path),
        'git_path': git_path,
        'python': python_path,
        'activation_hint_sh': f'source {emsdk_dir}/emsdk_env.sh',
        'activation_hint_ps1': f'& {emsdk_dir}\\emsdk_env.ps1',
        'activation_hint_bat': f'call {emsdk_dir}\\emsdk_env.bat',
    }
    write_reports(out_dir, info)
    print(f'[venom] Emscripten setup status: {status}')
    print(f'[venom] wrote setup report: {out_dir / "emscripten-setup.txt"}')
    if not emcc_path and not args.allow_missing:
        return 3
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


def run(cmd: list[str], *, expect: int = 0, timeout: int = 60) -> subprocess.CompletedProcess[str]:
    proc = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
    if proc.returncode != expect:
        print(proc.stdout)
        raise SystemExit(f'command returned {proc.returncode}, expected {expect}: {cmd}')
    return proc


def main() -> int:
    if len(sys.argv) != 3:
        print('usage: turbojs-wasm-cutover-workflow-smoke.py <repo-root> <out-dir>', file=sys.stderr)
        return 2
    root = Path(sys.argv[1]).resolve()
    out = Path(sys.argv[2]).resolve()
    out.mkdir(parents=True, exist_ok=True)

    setup_tool = root / 'tools' / 'setup_emscripten.py'
    setup_out = out / 'emscripten-setup'
    run([
        sys.executable, str(setup_tool),
        '--repo-root', str(root),
        '--out-dir', str(setup_out),
        '--check-only',
        '--allow-missing',
    ])
    setup_txt = (setup_out / 'emscripten-setup.txt').read_text(encoding='utf-8')
    for marker in ('VENOM_EMSCRIPTEN_SETUP_PREFLIGHT_V1', 'package_version=83', 'status='):
        if marker not in setup_txt:
            print(f'missing Emscripten setup marker {marker!r}', file=sys.stderr)
            return 1
    setup_json = json.loads((setup_out / 'emscripten-setup.json').read_text(encoding='utf-8'))
    if setup_json.get('package_version') != 83:
        print('unexpected setup package version', file=sys.stderr)
        return 1

    setup_script = root / 'scripts' / 'linux' / 'setup-emscripten.sh'
    run([str(setup_script), '--check-only', '--allow-missing', '--out-dir', str(out / 'setup-script')])
    if not (out / 'setup-script' / 'emscripten-setup.txt').exists():
        print('setup-emscripten.sh did not write setup report', file=sys.stderr)
        return 1

    cutover_tool = root / 'tools' / 'turbojs_wasm_cutover.py'
    cutover_out = out / 'cutover-preflight'
    run([
        sys.executable, str(cutover_tool),
        '--repo-root', str(root),
        '--out-dir', str(cutover_out),
        '--preflight-only',
        '--allow-missing-emcc',
    ])
    preflight_text = (cutover_out / 'turbojs-wasm-preflight.txt').read_text(encoding='utf-8')
    for marker in ('VENOM_TJS_WASM_PREFLIGHT_V1', 'package_version=83', 'missing_required_export_count=0'):
        if marker not in preflight_text:
            print(f'missing cutover preflight marker {marker!r}', file=sys.stderr)
            return 1

    verify_out = out / 'embedded-status'
    passed = subprocess.run([
        sys.executable, str(cutover_tool),
        '--repo-root', str(root),
        '--out-dir', str(verify_out),
        '--verify-embedded',
        '--require-real',
    ], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
    if passed.returncode != 0:
        print(passed.stdout)
        print('strict real-engine verification should pass for the checked-in upstream TurboJS WASM artifact', file=sys.stderr)
        return 1
    status = verify_out / 'embedded_status.txt'
    if not status.exists():
        print(passed.stdout)
        print('strict verification did not write embedded_status.txt', file=sys.stderr)
        return 1
    status_text = status.read_text(encoding='utf-8')
    for marker in ('VENOM_TJS_WASM_CUTOVER_EMBEDDED_STATUS_V1', 'artifact_kind=upstream-turbojs-wasm', 'strict_real=true'):
        if marker not in status_text:
            print(passed.stdout)
            print(f'missing embedded status marker {marker!r}', file=sys.stderr)
            return 1

    print('turbojs wasm cutover workflow smoke: PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

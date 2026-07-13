#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import json
import subprocess
import sys
from pathlib import Path


def run(cmd: list[str], *, timeout: int = 90) -> subprocess.CompletedProcess[str]:
    proc = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
    if proc.returncode != 0:
        print(proc.stdout)
        raise SystemExit(f'command failed: {cmd}')
    return proc


def main() -> int:
    if len(sys.argv) != 3:
        print('usage: emscripten-build-workflow-smoke.py <repo-root> <out-dir>', file=sys.stderr)
        return 2
    root = Path(sys.argv[1]).resolve()
    out = Path(sys.argv[2]).resolve()
    out.mkdir(parents=True, exist_ok=True)

    tool = root / 'tools' / 'build_emscripten.py'
    run([
        sys.executable,
        str(tool),
        '--repo-root', str(root),
        '--out-dir', str(out / 'tool'),
        '--preflight-only',
        '--allow-missing',
    ])
    text = (out / 'tool' / 'emscripten-build.txt').read_text(encoding='utf-8')
    for marker in (
        'VENOM_EMSCRIPTEN_BUILD_V1',
        'package_version=83',
        'status=preflight-ok',
        'required_export_count=',
        'missing_required_export_count=0',
    ):
        if marker not in text:
            print(f'missing build manifest marker {marker!r}', file=sys.stderr)
            return 1
    info = json.loads((out / 'tool' / 'emscripten-build.json').read_text(encoding='utf-8'))
    if info.get('package_version') != 83:
        print('unexpected Emscripten build package version', file=sys.stderr)
        return 1
    if not info.get('repo_inputs_ok'):
        print('repo inputs should pass in preflight mode', file=sys.stderr)
        return 1

    shell_script = root / 'scripts' / 'build-emscripten.sh'
    run([str(shell_script), '--preflight-only', '--allow-missing', '--out-dir', str(out / 'script')])
    if not (out / 'script' / 'emscripten-build.txt').exists():
        print('build-emscripten.sh did not write status manifest', file=sys.stderr)
        return 1

    docs = root / 'docs' / 'emscripten-build.md'
    doc_text = docs.read_text(encoding='utf-8')
    for marker in ('build-emscripten', 'setup/check emsdk', 'strict QuickJS WASM preflight'):
        if marker not in doc_text:
            print(f'missing docs marker {marker!r}', file=sys.stderr)
            return 1


    build_tool_text = tool.read_text(encoding='utf-8')
    forbidden = 'call "{env_bat}" &&'
    if forbidden in build_tool_text or "emsdk_env.bat'" in build_tool_text:
        print('build_emscripten.py should not chain through emsdk_env.bat on Windows', file=sys.stderr)
        return 1
    if 'build-quickjs-wasm.ps1' not in build_tool_text or "'-File'" not in build_tool_text:
        print('build_emscripten.py should invoke build-quickjs-wasm.ps1 directly on Windows', file=sys.stderr)
        return 1
    for marker in ('rebuild_native_compiler', 'verify_native_runtime_binding', '--skip-native-rebuild', '--skip-runtime-smoke'):
        if marker not in build_tool_text:
            print(f'build_emscripten.py should close the embedded-runtime/native-compiler binding: missing {marker!r}', file=sys.stderr)
            return 1

    spec = importlib.util.spec_from_file_location('venom_build_emscripten_smoke', tool)
    if spec is None or spec.loader is None:
        print('unable to import build_emscripten.py for command construction test', file=sys.stderr)
        return 1
    build_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(build_module)
    original_is_windows = build_module.is_windows
    original_which = build_module.shutil.which
    try:
        build_module.is_windows = lambda: True
        build_module.shutil.which = lambda name: 'C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe' if name == 'powershell.exe' else None
        expected_emcc = 'C:/venom/build/emsdk/upstream/emscripten/emcc.exe'
        windows_cmd = build_module.shell_build_command(root, out / 'emsdk', out / 'wasm', expected_emcc)
    finally:
        build_module.is_windows = original_is_windows
        build_module.shutil.which = original_which
    if '-Emcc' not in windows_cmd or windows_cmd[windows_cmd.index('-Emcc') + 1] != expected_emcc:
        print('Windows Emscripten controller must pass the resolved compiler through -Emcc', file=sys.stderr)
        return 1

    quote_doc = root / 'docs' / 'emscripten-windows-quoting-fix.md'
    if not quote_doc.exists() or 'quote characters as part of the executable name' not in quote_doc.read_text(encoding='utf-8'):
        print('missing Windows Emscripten quoting fix documentation', file=sys.stderr)
        return 1


    ps_text = (root / 'scripts' / 'build-quickjs-wasm.ps1').read_text(encoding='utf-8')
    for marker in ('Invoke-VenomExternal', '$LASTEXITCODE', 'New-LifecycleArguments', '$env:EMCC', '$resolvedEmccPath', 'emcc failed with exit code', 'quickjs_wasm_cutover.py', 'exit code ${code}:', '-sSTACK_SIZE=4194304'):
        if marker not in ps_text:
            print(f'build-quickjs-wasm.ps1 should enforce external command failure marker {marker!r}', file=sys.stderr)
            return 1

    shell_quickjs_text = (root / 'scripts' / 'build-quickjs-wasm.sh').read_text(encoding='utf-8')
    for marker in ('--detect-features', '--enable-bulk-memory-opt', '--enable-nontrapping-float-to-int'):
        if marker not in ps_text or marker not in shell_quickjs_text:
            print(f'QuickJS Binaryen post-processing must propagate WebAssembly feature marker {marker!r}', file=sys.stderr)
            return 1
    if '--all-features' in ps_text or '--all-features' in shell_quickjs_text:
        print('QuickJS Binaryen post-processing should enable only detected/required features, not all features', file=sys.stderr)
        return 1
    if '@wasmFeatureFlags' not in ps_text or '"${WASM_FEATURE_FLAGS[@]}"' not in shell_quickjs_text:
        print('QuickJS Binaryen feature arrays must be included in the actual wasm-opt command', file=sys.stderr)
        return 1

    wasm_exports_text = (root / 'tools' / 'wasm_exports.py').read_text(encoding='utf-8')
    for marker in ('WebAssembly.Module.imports', 'makeImportObject', 'defaultImportFor'):
        if marker not in wasm_exports_text:
            print(f'wasm_exports.py should synthesize import objects for runtime value probes: missing {marker!r}', file=sys.stderr)
            return 1

    package_release = (root / 'tools' / 'package_release.py').read_text(encoding='utf-8')
    if 'build_emscripten.py' not in package_release:
        print('release package should ship build_emscripten.py', file=sys.stderr)
        return 1

    print('emscripten build workflow smoke: PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

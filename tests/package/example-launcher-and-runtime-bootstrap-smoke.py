#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
sys.path.insert(0, str(root / 'tools'))
from venom_tools.examples import load_example_registry  # noqa: E402

ps = (root / 'tools' / 'windows-scripts' / 'build-turbojs-wasm.ps1').read_text(encoding='utf-8')
for marker in ('--verify-embedded', '--require-real', 'if($Artifact)', 'turbojs_runtime_lifecycle.py'):
    if marker not in ps:
        raise SystemExit(f'TurboJS Windows bootstrap missing marker: {marker}')
if "if($Artifact -eq ''){throw" in ps.replace(' ', ''):
    raise SystemExit('TurboJS artifact mode must not reject an explicitly supplied artifact')

launcher = (root / 'tools' / 'launch_example.py').read_text(encoding='utf-8')
for marker in ('load_example_registry', "'verify-runtime'", 'ThreadingHTTPServer', 'spec.chrome_extension'):
    if marker not in launcher:
        raise SystemExit(f'semantic example launcher missing marker: {marker}')

if "dist=site/'dist-venom'" not in launcher:
    raise SystemExit('example launcher must keep protected output inside each example directory')
if "root/f'dist-{name}-{profile}'" in launcher:
    raise SystemExit('example launcher must not emit protected output at repository root')

registry = load_example_registry(root)
for spec in registry.examples:
    linux = root / 'scripts' / 'linux' / f'build-and-launch-{spec.launcher}.sh'
    windows = root / 'scripts' / 'windows' / f'build-and-launch-{spec.launcher}.bat'
    if not linux.is_file() or not windows.is_file():
        raise SystemExit(f'missing semantic launchers for {spec.launcher}')
    linux_text = linux.read_text(encoding='utf-8')
    windows_text = windows.read_text(encoding='utf-8')
    if spec.launcher == 'react-vite-showcase':
        if 'npm run build' not in linux_text or 'npm run build' not in windows_text:
            raise SystemExit('React/Vite launcher must run the framework production build')
        continue
    if f'launch_example.py" {spec.launcher} ' not in linux_text:
        raise SystemExit(f'Linux launcher has wrong registry key: {linux.name}')
    if f'VENOM_EXAMPLE={spec.launcher}' not in windows_text:
        raise SystemExit(f'Windows launcher has wrong registry key: {windows.name}')
    if 'invoke-example.bat' not in windows_text:
        raise SystemExit(f'Windows launcher bypasses shared bootstrap: {windows.name}')

print(f'example launcher and runtime bootstrap smoke: PASS ({len(registry.examples)} examples)')

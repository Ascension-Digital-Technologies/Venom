#!/usr/bin/env python3
from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path


def project_version(root: Path) -> str:
    import re
    text = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
    match = re.search(r'project\(venom VERSION ([0-9]+\.[0-9]+\.[0-9]+)', text)
    if not match:
        raise RuntimeError('unable to read project version')
    return match.group(1)


def main() -> int:
    if len(sys.argv) != 4:
        print('usage: release-packaging-smoke.py <repo-root> <venom-exe> <out-dir>', file=sys.stderr)
        return 2
    root = Path(sys.argv[1]).resolve()
    venom = Path(sys.argv[2]).resolve()
    out = Path(sys.argv[3]).resolve()
    version_value = project_version(root)
    if out.exists():
        shutil.rmtree(out)

    script = root / 'tools' / 'package_release.py'
    result = subprocess.run(
        [sys.executable, str(script), '--repo-root', str(root), '--venom', str(venom), '--out', str(out)],
        text=True,
        capture_output=True,
        timeout=60,
    )
    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr, file=sys.stderr)
        return result.returncode

    binary = out / ('venom.exe' if os.name == 'nt' else 'venom')
    required = [
        binary,
        out / 'README.md',
        out / 'VERSION.txt',
        out / 'RELEASE_MANIFEST.txt',
        out / 'docs' / 'security-model.md',
        out / 'docs' / 'release-packaging.md',
        out / 'docs' / 'release-signing.md',
        out / 'examples' / 'protected-chess' / 'index.html',
        out / 'scripts' / 'build-site.sh',
        out / 'scripts' / 'package-release.sh',
        out / 'scripts' / 'verify-release.sh',
        out / 'tools' / 'verify_release.py',
        out / 'tools' / 'quickjs_wasm_preflight.py',
        out / 'tools' / 'quickjs_wasm_cutover.py',
        out / 'tools' / 'setup_emscripten.py',
        out / 'licenses' / 'QUICKJS-LICENSE',
    ]
    missing = [str(p.relative_to(out)) for p in required if not p.exists()]
    if missing:
        print('missing release files: ' + ', '.join(missing), file=sys.stderr)
        return 1

    version = (out / 'VERSION.txt').read_text(encoding='utf-8')
    if f'venom {version_value}' not in version:
        print('VERSION.txt did not record the current project version', file=sys.stderr)
        return 1

    manifest = (out / 'RELEASE_MANIFEST.txt').read_text(encoding='utf-8')
    for needle in ('VENOM_RELEASE_MANIFEST_V1', 'venom', 'docs/security-model.md', 'docs/release-signing.md', 'examples/protected-chess/index.html', 'scripts/build.sh', 'scripts/package-release.sh', 'scripts/verify-release.sh', 'tools/verify_release.py', 'tools/quickjs_wasm_preflight.py', 'tools/quickjs_wasm_cutover.py', 'tools/setup_emscripten.py'):
        if needle not in manifest:
            print(f'RELEASE_MANIFEST.txt missing {needle!r}', file=sys.stderr)
            return 1

    check = subprocess.run([str(binary), '--version'], text=True, capture_output=True, timeout=20)
    if check.returncode != 0 or f'venom {version_value}' not in (check.stdout + check.stderr):
        print('release binary --version failed', file=sys.stderr)
        print(check.stdout)
        print(check.stderr, file=sys.stderr)
        return 1

    verify = subprocess.run([sys.executable, str(root / 'tools' / 'verify_release.py'), str(out), '--expect-version', version_value], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
    if verify.returncode != 0 or 'release verification: PASS' not in verify.stdout:
        print(verify.stdout)
        print('release verifier failed on unsigned release directory', file=sys.stderr)
        return 1

    print('release packaging smoke: PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

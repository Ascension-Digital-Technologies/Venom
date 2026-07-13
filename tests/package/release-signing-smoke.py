#!/usr/bin/env python3
from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path


def run(cmd: list[str], *, expect: int = 0, timeout: int = 90) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
    if result.returncode != expect:
        print(result.stdout)
        raise SystemExit(f'command returned {result.returncode}, expected {expect}: {cmd!r}')
    return result


def project_version(root: Path) -> str:
    import re
    text = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
    match = re.search(r'project\(venom VERSION ([0-9]+\.[0-9]+\.[0-9]+)', text)
    if not match:
        raise RuntimeError('unable to read project version')
    return match.group(1)


def main() -> int:
    if len(sys.argv) != 4:
        print('usage: release-signing-smoke.py <repo-root> <venom-exe> <out-dir>', file=sys.stderr)
        return 2
    root = Path(sys.argv[1]).resolve()
    venom = Path(sys.argv[2]).resolve()
    out = Path(sys.argv[3]).resolve()
    key = 'venom-release-signing-smoke-key'
    version_value = project_version(root)
    if out.exists():
        shutil.rmtree(out)
    archive = out.with_name(f'venom-v{version_value}-{sys.platform.lower()}-{os.uname().machine.lower()}.zip') if hasattr(os, 'uname') else None
    # The packager uses platform.system().lower(), not sys.platform, so remove any old matching archive broadly.
    for old in out.parent.glob(f'venom-v{version_value}-*.zip'):
        old.unlink()

    packager = root / 'tools' / 'package_release.py'
    verifier = root / 'tools' / 'verify_release.py'
    run([
        sys.executable, str(packager),
        '--repo-root', str(root),
        '--venom', str(venom),
        '--out', str(out),
        '--archive', 'zip',
        '--sign', 'dev-sha256',
        '--dev-insecure-key', key,
    ])

    required = [
        out / 'RELEASE_MANIFEST.txt',
        out / 'RELEASE_MANIFEST.sig',
        out / 'tools' / 'verify_release.py',
        out / 'scripts' / 'verify-release.sh',
    ]
    missing = [str(p.relative_to(out)) for p in required if not p.exists()]
    if missing:
        raise SystemExit('missing signed release files: ' + ', '.join(missing))

    sig = (out / 'RELEASE_MANIFEST.sig').read_text(encoding='utf-8')
    if 'VENOM_RELEASE_SIGNATURE_V1' not in sig or 'type=dev-sha256' not in sig:
        raise SystemExit('signature file does not contain expected dev-sha256 signature metadata')

    manifest = (out / 'RELEASE_MANIFEST.txt').read_text(encoding='utf-8')
    for needle in ('tools/verify_release.py', 'scripts/verify-release.sh', 'docs/release-signing.md'):
        if needle not in manifest:
            raise SystemExit(f'manifest missing {needle!r}')

    run([sys.executable, str(verifier), str(out), '--expect-version', version_value, '--strict-signature', '--dev-insecure-key', key])

    archives = sorted(out.parent.glob(f'venom-v{version_value}-*.zip'))
    if len(archives) != 1:
        raise SystemExit(f'expected one signed release archive, found {len(archives)}')
    run([sys.executable, str(verifier), str(archives[0]), '--expect-version', version_value, '--strict-signature', '--dev-insecure-key', key])

    # Integrity verifier must catch post-manifest tampering.
    version_file = out / 'VERSION.txt'
    version_file.write_text(version_file.read_text(encoding='utf-8') + '\ntampered=true\n', encoding='utf-8')
    tampered = subprocess.run(
        [sys.executable, str(verifier), str(out), '--expect-version', version_value, '--strict-signature', '--dev-insecure-key', key],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=60,
    )
    if tampered.returncode == 0 or 'sha256 mismatch' not in tampered.stdout:
        print(tampered.stdout)
        raise SystemExit('release verifier did not reject tampered release')

    print('release signing smoke: PASS')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

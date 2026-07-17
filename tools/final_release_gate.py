#!/usr/bin/env python3
"""Fail-fast repository gate for a signed, internally consistent Venom release."""
from __future__ import annotations
import argparse, re, subprocess, sys
from pathlib import Path

REQUIRED = [
    'CMakeLists.txt', 'README.md', 'CHANGES.md',
    'scripts/windows/release.ps1', 'scripts/linux/release.sh',
    '.github/workflows/release.yml', '.github/workflows/release-hardening.yml',
    'tools/package_release_set.py', 'tools/verify_release_set.py', 'tools/documentation_gate.py',
    'tools/certify_release.py', 'tools/aggregate_certification.py',
    'contracts/release-certification.json', '.github/workflows/certification.yml',
    'docs/operations/release-certification.md',
    'contracts/runtime-api.json', 'contracts/final-release.json', 'tools/final_readiness_report.py',
    'packages/runtime/package.json',
    'packages/runtime/src/index.js', 'packages/runtime/src/index.d.ts',
    'cmake/build_acceleration.cmake',
    'docs/README.md', 'docs/getting-started/build-from-source.md',
    'docs/operations/release-verification.md', 'docs/security/security-model.md',
    'src/generated/runtime/quickjs_runtime_wasm_blob.hpp',
    'src/generated/runtime/wasm_runtime_provenance.json',
]

def fail(message: str) -> None:
    print(f'[FAIL] {message}')
    raise SystemExit(1)

def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument('--run-smoke-tests', action='store_true')
    a=ap.parse_args(); root=a.repo_root.resolve()
    for rel in REQUIRED:
        if not (root/rel).is_file(): fail(f'missing required release input: {rel}')
    cmake=(root/'CMakeLists.txt').read_text(encoding='utf-8')
    m=re.search(r'VERSION\s+(\d+\.\d+\.\d+)',cmake)
    if not m: fail('project version not found in CMakeLists.txt')
    version=m.group(1)
    readme=(root/'README.md').read_text(encoding='utf-8')
    changes=(root/'CHANGES.md').read_text(encoding='utf-8')
    readme_version_markers = (f'**Version:** {version}', f'Version {version}</strong>')
    if not any(marker in readme for marker in readme_version_markers): fail('README version does not match CMake project version')
    if f'## {version} ' not in changes: fail('changelog lacks current version heading')
    release=(root/'.github/workflows/release.yml').read_text(encoding='utf-8')
    hardening=(root/'.github/workflows/release-hardening.yml').read_text(encoding='utf-8')
    for token in ('--require-signature','RELEASE_SET.sig','VENOM_RELEASE_PRIVATE_KEY_PEM','VENOM_RELEASE_PUBLIC_KEY_PEM'):
        if token not in release: fail(f'release workflow lacks signed-publication contract: {token}')
    if "tags: ['v*']" in hardening: fail('release-hardening workflow must not independently trigger on tags')
    tag_owners=0
    for wf in (root/'.github/workflows').glob('*.yml'):
        if "tags: ['v*']" in wf.read_text(encoding='utf-8'): tag_owners += 1
    if tag_owners != 1: fail(f'exactly one tag publication workflow is required; found {tag_owners}')
    subprocess.run([sys.executable, str(root/'tools/documentation_gate.py')], check=True, cwd=root)
    subprocess.run([sys.executable, str(root/'tests/package/changelog-uniqueness-smoke.py')], check=True, cwd=root)
    subprocess.run([sys.executable, str(root/'tests/package/release-entrypoint-policy-smoke.py')], check=True, cwd=root)
    subprocess.run([sys.executable, str(root/'tests/package/cmake-module-completeness-smoke.py'), str(root)], check=True, cwd=root)
    subprocess.run([sys.executable, str(root/'tests/package/cmake-source-completeness-smoke.py'), str(root)], check=True, cwd=root)
    if a.run_smoke_tests:
        subprocess.run([sys.executable,str(root/'tests/package/release-publication-set-smoke.py')],check=True,cwd=root)
        subprocess.run([sys.executable,str(root/'tests/package/verified-runtime-release-chain-smoke.py')],check=True,cwd=root)
    print(f'[PASS] final release gate: version={version} signed_tag_owner=1')
    return 0
if __name__=='__main__': raise SystemExit(main())

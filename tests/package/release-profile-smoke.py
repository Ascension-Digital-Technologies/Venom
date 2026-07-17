#!/usr/bin/env python3
import pathlib
import subprocess
import sys

if len(sys.argv) != 6:
    print('usage: release-profile-smoke.py <venom> <site> <browser-out> <native-out> <key-file>')
    raise SystemExit(2)

venom = pathlib.Path(sys.argv[1])
site = pathlib.Path(sys.argv[2])
browser_out = pathlib.Path(sys.argv[3])
native_out = pathlib.Path(sys.argv[4])
key_file = pathlib.Path(sys.argv[5])

subprocess.run([str(venom), 'build', str(site), '--out', str(browser_out), '--profile', 'prod'], check=True)
browser_check = subprocess.run([str(venom), 'release-check', str(browser_out), '--target', 'browser'], check=True, text=True, capture_output=True)
browser_required = [
    'release_profile: yes',
    'threat_model: browser-client-protection-v1',
    'secret_model: browser-runtime-no-hidden-secret',
    'release_profile_audited_crypto: no',
    'release_profile_external_key_required: no',
    'release_status: PASS',
]
for marker in browser_required:
    if marker not in browser_check.stdout:
        raise SystemExit(f'missing browser release profile marker: {marker}\n{browser_check.stdout}')

browser_pkg = next((browser_out / 'assets').glob('app*.vbc'))
browser_raw = browser_pkg.read_bytes()
for marker in [b'VENOM_RELEASE_PROFILE_V1', b'release-profile.vrpf', b'browser-client-protection-v1', b'browser-runtime-no-hidden-secret']:
    if marker in browser_raw:
        raise SystemExit(f'raw browser package leaked release profile marker: {marker!r}')

subprocess.run([str(venom), 'keygen', '--out', str(key_file), '--force'], check=True)
native_build = subprocess.run([str(venom), 'build', str(site), '--out', str(native_out), '--profile', 'prod', '--key-file', str(key_file), '--require-audited-crypto'], text=True, capture_output=True)
if native_build.returncode != 0:
    if 'libsodium provider is not available' in native_build.stdout + native_build.stderr:
        print('libsodium unavailable; release-profile native path skipped')
        raise SystemExit(0)
    print(native_build.stdout)
    print(native_build.stderr)
    raise SystemExit(native_build.returncode)

native_check = subprocess.run([str(venom), 'release-check', str(native_out), '--target', 'native', '--key-file', str(key_file), '--require-audited-crypto'], check=True, text=True, capture_output=True)
native_required = [
    'release_profile: yes',
    'threat_model: native-private-aead-v1',
    'secret_model: external-32-byte-package-key-required',
    'release_profile_audited_crypto: yes',
    'release_profile_external_key_required: yes',
    'release_status: PASS',
]
for marker in native_required:
    if marker not in native_check.stdout:
        raise SystemExit(f'missing native release profile marker: {marker}\n{native_check.stdout}')

native_pkg = next((native_out / 'assets').glob('app*.vbc'))
native_raw = native_pkg.read_bytes()
for marker in [b'VENOM_RELEASE_PROFILE_V1', b'release-profile.vrpf', b'native-private-aead-v1', b'external-32-byte-package-key-required']:
    if marker in native_raw:
        raise SystemExit(f'raw native package leaked release profile marker: {marker!r}')

print('release profile smoke passed')

import subprocess
import sys
from pathlib import Path

venom = Path(sys.argv[1])
site = Path(sys.argv[2])
out = Path(sys.argv[3])

subprocess.run([str(venom), 'build', str(site), '--out', str(out), '--profile', 'prod', '--hashed'], check=True)
check = subprocess.run([str(venom), 'verify-runtime', str(out), '--target', 'browser'], check=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
for marker in (
    'Runtime verification report:',
    'turbojs_runtime_implementation: turbojs-wasm-upstream-turbojs',
    'turbojs_runtime_contract_only: no',
    'turbojs_runtime_scaffold: no',
    'turbojs_runtime_real_engine_candidate: yes',
    'turbojs_runtime_full_upstream_turbojs: yes',
    'turbojs_runtime_finish_blocker: none',
    'turbojs_runtime_artifact_kind: upstream-turbojs-wasm',
    'turbojs_runtime_required_exports_satisfied: yes',
    'turbojs_runtime_package_version: 83',
    'release_status: PASS',
):
    if marker not in check.stdout:
        raise SystemExit(f'missing verify-runtime marker {marker!r}\n{check.stdout}')

strict = subprocess.run([str(venom), 'verify-runtime', str(out), '--target', 'browser', '--require-real-engine'], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
if strict.returncode != 0:
    raise SystemExit(f'verify-runtime --require-real-engine must pass for the checked-in real TurboJS WASM artifact\n{strict.stdout}')
if 'release_status: PASS' not in strict.stdout or 'turbojs_runtime_full_upstream_turbojs: yes' not in strict.stdout:
    raise SystemExit(f'verify-runtime strict output is missing real-engine markers\n{strict.stdout}')
print('verify-runtime smoke passed')

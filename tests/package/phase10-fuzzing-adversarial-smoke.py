#!/usr/bin/env python3
from pathlib import Path
import re
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()

def project_version(text):
    match = re.search(r'project\(venom\s+VERSION\s+(\d+)\.(\d+)\.(\d+)', text, re.S)
    return tuple(map(int, match.groups())) if match else (0, 0, 0)
cmake = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
cli = (root / 'src/compiler/cli.cpp').read_text(encoding='utf-8')
gate = (root / 'tools/run_fuzz_gate.py').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/release-hardening.yml').read_text(encoding='utf-8')
checks = {
    'version': project_version(cmake) >= (1, 0, 23),
    'package fuzzer': 'venom_package_parser_fuzz' in cmake,
    'contract fuzzer': 'venom_runtime_contract_fuzz' in cmake,
    'differential fuzzer': 'venom_package_parser_differential_fuzz' in cmake,
    'contract replay': 'venom_runtime_contract_replay' in cmake,
    'deterministic mutation gate': 'venom-fuzz-gate-v1' in gate and 'mutations-per-seed' in gate,
    'package corpus': (root / 'tests/fuzz/corpus/package_parser').is_dir(),
    'contract corpus': (root / 'tests/fuzz/corpus/runtime_contracts').is_dir(),
    'ci fuzz gate': 'Fuzz and hostile-input gates' in workflow,
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    print('phase10 smoke failed: ' + ', '.join(failed), file=sys.stderr)
    raise SystemExit(1)
print('phase10 fuzzing/adversarial smoke passed')

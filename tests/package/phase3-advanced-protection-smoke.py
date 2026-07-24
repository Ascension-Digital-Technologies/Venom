#!/usr/bin/env python3
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]

def project_version(text):
    match = re.search(r'project\(venom\s+VERSION\s+(\d+)\.(\d+)\.(\d+)', text, re.S)
    return tuple(map(int, match.groups())) if match else (0, 0, 0)
build = (root / 'src/pipeline/build.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_package_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_audit_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_module_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/planning/section_plan.cpp').read_text(encoding='utf-8')
crypto = (root/'src/package/crypto.cpp').read_text(encoding='utf-8')
analyzer = (root/'tools/analyze_dist.py').read_text(encoding='utf-8')
cmake = (root/'CMakeLists.txt').read_text(encoding='utf-8')
checks = {
    'v1.0.16+': project_version(cmake) >= (1, 0, 16),
    'binary diversification magic': "{'V','R','D','I','V','0','0','3'}" in build,
    'host-call permutation': 'std::shuffle(host_ids.begin(), host_ids.end(), host_rng)' in build,
    'result-field permutation': 'std::shuffle(field_ids.begin(), field_ids.end(), field_rng)' in build,
    'release package section': 'release-diversification.vrd3' in build,
    'key wipe guard': 'ScopedKeyWipe key_wipe{key};' in crypto,
    'decoded key text wipe': 'secure_wipe(hex.data(), hex.size())' in crypto,
    'reconnaissance score': 'reconnaissance_score' in analyzer,
    'strict score gate': 'reconnaissance score below strict release threshold' in analyzer,
}
failed=[k for k,v in checks.items() if not v]
if failed:
    raise SystemExit('phase3 smoke failed: '+', '.join(failed))
print('phase3 advanced protection smoke: PASS')

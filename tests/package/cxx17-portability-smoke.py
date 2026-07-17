#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
source = '\n'.join(p.read_text(encoding='utf-8', errors='ignore') for p in (root/'src').rglob('*.cpp'))
assert '.ends_with(' not in source, 'std::string::ends_with requires C++20'
assert 'for (const auto& [digest, paths] : duplicate_groups)' not in source
assert 'for (const auto& [exported_name, local_name] : file.local_export_bindings)' not in source
print('C++17 portability smoke: PASS')

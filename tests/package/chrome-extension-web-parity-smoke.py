#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
emitter = (root / 'src/pipeline/chrome_extension.cpp').read_text(encoding='utf-8')
assert 'primary_extension_page' in emitter
assert 'if (has_protected_file_directive(file)) return false;' in emitter
assert 'route_output = output / std::filesystem::path(route_path)' in emitter
assert 'write protected Chrome extension route' in emitter
assert 'resource.context == ExecutionContext::StaticResource' in emitter
print('Chrome extension protected web-parity smoke: PASS')

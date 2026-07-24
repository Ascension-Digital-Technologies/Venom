#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
checks = {
    root / 'src/core/options.hpp': ['harden_public_javascript = true'],
    root / 'src/core/config.cpp': ['build.harden_public_javascript', 'protection.harden_public_javascript'],
    root / 'src/cli/cli.cpp': ['--harden-public-js', '--no-harden-public-js'],
    root / 'src/pipeline/js_bundle_encoding.cpp': ['"browser-module"', '"browser-script"', 'harden_public_javascript'],
    root / 'src/pipeline/chrome_extension.cpp': ['harden_public_javascript', 'harden_extension_js'],
    root / 'src/pipeline/native_js_hardener.cpp': ['browserFacingKind', "kind.startsWith('browser-')"],
    root / 'venom.toml.example': ['harden_public_javascript = true'],
}
for path, markers in checks.items():
    text = path.read_text(encoding='utf-8')
    missing = [m for m in markers if m not in text]
    if missing:
        raise SystemExit(f'{path}: missing {missing}')
print('public JavaScript hardening policy smoke passed')

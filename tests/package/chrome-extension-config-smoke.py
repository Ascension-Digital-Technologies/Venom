#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
config = (root / 'src/core/config.cpp').read_text(encoding='utf-8')
cli = (root / 'src/cli/cli.hpp').read_text(encoding='utf-8')
build = (root / 'src/pipeline/build.cpp').read_text(encoding='utf-8')
assert 'project.target' in config
assert 'chrome-extension' in config
assert 'project_target = "web"' in cli
assert 'chrome_extension::validate_project' in build
assert 'chrome_extension::emit_extension_files' in build
print('Chrome extension config smoke: PASS')

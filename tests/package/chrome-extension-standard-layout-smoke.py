#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
assets = (root / 'src/pipeline/assets.cpp').read_text(encoding='utf-8')
build = (root / 'src/pipeline/build.cpp').read_text(encoding='utf-8')
emitter = (root / 'src/pipeline/chrome_extension.cpp').read_text(encoding='utf-8')
inspection = (root / 'src/pipeline/security_artifact_inspection.cpp').read_text(encoding='utf-8')

assert 'if (standard_layout_only) return false;' in assets
assert 'production_hardening, chrome_extension_target' in build
assert 'constexpr std::string_view k_extension_shell_dir = "assets/extension";' in emitter
assert 'relocate_manifest_paths' in emitter
assert 'relocate_chrome_api_literals' in emitter
assert 'relocate_engine_asset_urls' in emitter
assert 'emitted_extension_path(file.relative, analysis)' in emitter
assert 'shell_path("venom-extension-rpc.js")' in emitter
assert 'primary_extension_page(analysis)' in emitter
assert '"$1assets/extension/venom-background.js$2"' in emitter
assert '"assets/extension/engine.html"' in inspection
assert 'html_path.parent_path()' in inspection
assert 'failed to remove temporary Chrome extension index.html' in emitter
print('Chrome extension standard layout smoke: PASS')

assert 'path.rfind(shell_prefix, 0) == 0' in emitter

assert 'emitted_extension_path(path, analysis)' in emitter

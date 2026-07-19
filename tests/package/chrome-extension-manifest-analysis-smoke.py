#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
hpp = (root / 'src/pipeline/include/chrome_extension.hpp').read_text(encoding='utf-8')
cpp = (root / 'src/pipeline/chrome_extension.cpp').read_text(encoding='utf-8')
assert 'enum class ExecutionContext' in hpp
assert 'ContentScriptMain' in hpp and 'ServiceWorker' in hpp and 'SandboxPage' in hpp
assert 'ManifestAnalysis analyze_project' in hpp
for key in ['action', 'background', 'content_scripts', 'options_ui', 'side_panel', 'devtools_page', 'sandbox', 'declarative_net_request', 'web_accessible_resources']:
    assert f'"{key}"' in cpp
assert 'getURL' in cpp
assert 'script_file' in cpp
assert 'Traverse from manifest entrypoints only' in cpp
assert 'Chrome extension resource is missing:' in cpp
print('Chrome extension manifest analysis source smoke: PASS')

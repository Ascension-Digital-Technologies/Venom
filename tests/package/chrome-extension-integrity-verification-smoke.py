#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
inspection = (root / 'src/pipeline/security_artifact_inspection.cpp').read_text(encoding='utf-8')
inspection_hpp = (root / 'src/pipeline/security_artifact_inspection.hpp').read_text(encoding='utf-8')
analysis = (root / 'src/pipeline/security_analysis.cpp').read_text(encoding='utf-8')

assert 'verification_html_path' in inspection_hpp
assert '{"index.html", "popup.html", "engine.html", "extension/engine.html", "assets/extension/engine.html"}' in inspection
assert 'std::filesystem::recursive_directory_iterator(dist_root)' in inspection
assert 'text.find("integrity=\\"sha256-")' in inspection
assert 'const auto html_path = verification_html_path(report.dist_root);' in analysis
assert 'production dist is missing a protected bootstrap HTML page for SRI verification' in analysis
assert 'production dist is missing index.html for SRI verification' not in analysis
assert 'protected bootstrap HTML is missing loader subresource integrity' in analysis
assert 'protected bootstrap HTML is missing stylesheet subresource integrity' in analysis
assert 'loader_path.lexically_relative(html_path.parent_path()).generic_string()' in analysis
assert 'style_path.lexically_relative(html_path.parent_path()).generic_string()' in analysis
assert 'public_asset_path(loader_path, report.dist_root)' not in analysis
assert 'public_asset_path(style_path, report.dist_root)' not in analysis
print('Chrome extension integrity verification smoke: PASS')

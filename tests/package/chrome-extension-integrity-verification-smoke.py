#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
inspection = (root / 'src/pipeline/security_artifact_inspection.cpp').read_text(encoding='utf-8')
inspection_hpp = (root / 'include/venom/internal/pipeline/security_artifact_inspection.hpp').read_text(encoding='utf-8')
analysis = (root / 'src/pipeline/security_analysis.cpp').read_text(encoding='utf-8')

assert 'verification_html_path' in inspection_hpp
assert '{"index.html", "engine.html", "extension/engine.html", "assets/extension/engine.html"}' in inspection
assert 'if (!extract_loader_binding_token(text).empty()) return entry.path();' in inspection
assert 'const auto html_path = verification_html_path(report.dist_root);' in analysis
assert 'production dist is missing a protected bootstrap HTML page for SRI verification' in analysis
assert 'production dist is missing index.html for SRI verification' not in analysis
assert 'protected bootstrap HTML is missing loader subresource integrity' in analysis
assert 'protected bootstrap HTML is missing stylesheet subresource integrity' in analysis
print('Chrome extension integrity verification smoke: PASS')

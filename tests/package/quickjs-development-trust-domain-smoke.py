#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
build = (root / 'src/compiler/pipeline/build.cpp').read_text(encoding='utf-8')
engine = (root / 'src/generated/runtime/quickjs_engine_module.cpp').read_text(encoding='utf-8')

assert 'const bool release_trust_domain = profile.kind == ProfileKind::Prod || options.strict_release;' in build
assert 'make_quickjs_engine_module_js(release_trust_domain)' in build
assert 'make_quickjs_engine_module_js(hardened_release_asset)' not in build
assert "!protectedRelease && handoff.producer === 'development-quickjs-compiler'" in engine
print('QuickJS development trust domain: PASS')

#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
emitter = (root / "src/pipeline/chrome_extension.cpp").read_text(encoding="utf-8")
hardener = (root / "src/pipeline/native_js_hardener.cpp").read_text(encoding="utf-8")

assert 'k_extension_shell_dir = "assets/extension"' in emitter
assert 'harden_extension_js(file.relative' in emitter
for name in ("venom-extension-rpc.js", "venom-extension-host.js", "venom-extension-broker.js", "venom-background.js"):
    assert f'harden_extension_js("{name}"' in emitter
assert 'extension-module' in hardener
assert 'extension-script' in emitter
assert 'extension-worker' in emitter
assert 'const auto extension_build_salt' in emitter
print("Chrome extension JavaScript hardening smoke: PASS")

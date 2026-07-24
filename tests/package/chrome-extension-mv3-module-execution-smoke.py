#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
runtime = (root / 'src/templates/browser/50-browser-modules.js').read_text()
emitter = (root / 'src/pipeline/chrome_extension.cpp').read_text()
assert "location.protocol === 'chrome-extension:'" in runtime
assert 'chromeExtension: true' in runtime
assert 'extension_page_adapter_js' in emitter
assert 'waitForVenom' in emitter
assert 'waitForApplication' in emitter
assert "const api=await waitForVenom();await api.ready();await waitForApplication();" in emitter
assert 'venom:boot-ready' in emitter
assert 'Venom protected route hydration timed out' in emitter
assert "Venom protected runtime is unavailable" not in emitter
assert "await import(chrome.runtime.getURL('" in emitter
assert 'include_custom_host' in emitter
assert 'engine_host_html(relocate_engine_asset_urls(protected_shell_html), analysis.runtime_host, false)' in emitter
print('Chrome MV3 module execution smoke: PASS')

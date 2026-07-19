from pathlib import Path
root=Path(__file__).resolve().parents[2]
browser=root/'src/templates/browser'
engine=root/'src/templates/quickjs-engine'
expected_browser={
 '00-contracts.js','10-package-loader.js','20-runtime-contracts.js','30-host-queues.js',
 '40-quickjs-bridge.js','50-browser-modules.js','60-route-runtime.js'}
expected_engine={
 '00-module-contract.js','10-wasm-memory.js','20-context-execution.js',
 '30-module-linking.js','40-diagnostics-surface.js'}
assert {p.name for p in browser.glob('*.js')}==expected_browser
assert {p.name for p in engine.glob('*.js')}==expected_engine
for p in list(browser.glob('*.js'))+list(engine.glob('*.js')):
    assert p.stat().st_size>100, f'empty runtime module: {p}'
assert 'function loadPackage' in (browser/'10-package-loader.js').read_text()
assert 'function createQuickJsEngine' in (browser/'40-quickjs-bridge.js').read_text()
assert 'function createBrowserModuleLinker' in (browser/'50-browser-modules.js').read_text()
assert 'async function executeWithWasmScaffold' in (engine/'20-context-execution.js').read_text()
assert 'function transformModuleSource' in (engine/'30-module-linking.js').read_text()
print('runtime module ownership: PASS')

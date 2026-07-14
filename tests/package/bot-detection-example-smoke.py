from pathlib import Path
root = Path(__file__).resolve().parents[2]
site = root / 'examples' / 'bot-detection'
for rel in ('index.html','README.md','venom.browser.json','venom.lock','venom.toml','assets/js/app.js','assets/js/fingerprint.js','protected/bot-engine.js'):
    assert (site / rel).is_file(), rel
html = (site / 'index.html').read_text(encoding='utf-8')
engine = (site / 'protected/bot-engine.js').read_text(encoding='utf-8')
app = (site / 'assets/js/app.js').read_text(encoding='utf-8')
assert '// @venom: protected module' in engine
assert 'export ' in engine
assert '// @venom: browser' in app
assert 'runtime.call("assessClient"' in app
assert 'protected/bot-engine.js' in html
for ext in ('ps1','bat','sh'):
    assert (root / 'scripts' / f'bot-detection.{ext}').is_file()
print('bot detection example smoke: PASS')

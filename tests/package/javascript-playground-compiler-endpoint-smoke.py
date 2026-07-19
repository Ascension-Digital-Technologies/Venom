from pathlib import Path
root = Path(__file__).resolve().parents[2]
launcher = (root / 'tools' / 'launch_example.py').read_text(encoding='utf-8')
app = (root / 'examples' / 'javascript-playground' / 'browser' / 'app.js').read_text(encoding='utf-8')
assert '/__venom/playground/compile' in launcher
assert 'compile-snippet' in launcher
assert '/__venom/playground/compile' in app
assert 'bytecodeBytes' in app
assert 'development-quickjs-compiler' in app
assert 'code: ""' in app
assert 'bytecode: false' not in app
print('javascript playground compiler endpoint: PASS')

from pathlib import Path
root = Path(__file__).resolve().parents[2]
app = (root / 'examples/javascript-playground/browser/app.js').read_text(encoding='utf-8')
examples = (root / 'examples/javascript-playground/browser/examples.js').read_text(encoding='utf-8')
assert 'normalize(await eval(${JSON.stringify(source)}))' in app
assert 'async function (input, console)' not in app
assert 'value.type === "undefined"' in app
assert 'return { total:' not in examples
assert '({ total:' in examples
assert 'primes;`' in examples
print('javascript playground completion value: PASS')

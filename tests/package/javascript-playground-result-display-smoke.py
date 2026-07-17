from pathlib import Path
root = Path(__file__).resolve().parents[2]
app = (root / 'examples/javascript-playground/browser/app.js').read_text(encoding='utf-8')
assert 'function bridgeEnvelope(report)' in app
assert 'Object.prototype.hasOwnProperty.call(envelope, "result")' in app
assert '__venomPlaygroundType: "undefined"' in app
assert 'report.evalResult ?? report.report' not in app
print('javascript playground result display: PASS')

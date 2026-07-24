from pathlib import Path
root=Path(__file__).resolve().parents[2]
app=(root/'examples/javascript-playground/browser/app.js').read_text(encoding='utf-8')
engine=(root/'src/generated/runtime/turbojs_engine_module.cpp').read_text(encoding='utf-8')
required_app=[
  '/__venom/playground/session','X-Venom-Playground-Token','executionPolicy',
  'isolation: "ephemeral"','heapLimitBytes','stackLimitBytes','interruptBudget',
  'maxResultBytes','maxBridgeInputBytes','consoleEvents','consoleBytes','type: "circular"'
]
for token in required_app:
    assert token in app, f'missing playground isolation token: {token}'
required_engine=[
  'const executionPolicy =','effectiveHeapLimit','effectiveStackLimit',
  'effectiveInterruptBudget','effectivePendingJobLimit','maxBridgeInputBytes','maxResultBytes',
  'TurboJS bridge result exceeds configured result limit'
]
for token in required_engine:
    assert token in engine, f'missing runtime isolation enforcement: {token}'
print('JavaScript playground isolation policy: PASS')

from pathlib import Path
import sys
root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
text = (root/'src/generated/runtime/runtime_js.cpp').read_text(encoding='utf-8')
text += (root / 'src/generated/runtime/javascript/browser_runtime.js').read_text(encoding='utf-8')
required = [
  "lateLifecycleHandlers",
  "type === 'DOMContentLoaded'",
  "document.readyState !== 'loading'",
  "type === 'load'",
  "record.listener.call(record.target, event)",
]
missing=[x for x in required if x not in text]
if missing:
  raise SystemExit('missing route lifecycle replay markers: '+', '.join(missing))
print('route lifecycle replay smoke: PASS')

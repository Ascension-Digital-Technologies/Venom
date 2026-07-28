#!/usr/bin/env python3
from pathlib import Path
import re
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
example = root / 'examples' / 'vite-framework-showcase' / 'src'
main = (example / 'main.ts').read_text(encoding='utf-8')
protected = (example / 'protected-score.ts').read_text(encoding='utf-8')

if not re.search(r'^\s*//\s*@venom:\s*protected\s+module\s*$', protected, re.MULTILINE | re.IGNORECASE):
    raise SystemExit('Vite protected dependency must use the file-scope protected module directive')
if not re.search(r'import\s*\{\s*calculateProtectedScore\s*\}\s*from\s*[\'\"]\.\/protected-score[\'\"]', main):
    raise SystemExit('Vite browser entry no longer exercises an extensionless protected-module import')
if re.search(r'^\s*//\s*@venom:\s*protected\s*$', protected, re.MULTILINE | re.IGNORECASE):
    raise SystemExit('ambiguous function-only protected directive returned to Vite module example')
main_match = re.search(r'async\s+function\s+main\s*\(', main)
await_match = re.search(r'\bawait\s+calculateProtectedScore\s*\(', main)
if not main_match:
    raise SystemExit('Vite browser entry must keep async startup inside an explicit function')
if not await_match or await_match.start() < main_match.start():
    raise SystemExit('Vite browser entry must not use top-level await with the default production target')
print('Vite protected-module example smoke: PASS')

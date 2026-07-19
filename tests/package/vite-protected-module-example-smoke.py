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
print('Vite protected-module example smoke: PASS')

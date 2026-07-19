#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[2]
s=(root/'src/remote/remote.cpp').read_text(encoding='utf-8')
assert '"--location"' in s
assert '"--proto-redir", "=https"' in s
assert '"--max-redirs", "8"' in s
assert '"--max-redirs", "0"' not in s
print('remote redirect policy smoke passed')

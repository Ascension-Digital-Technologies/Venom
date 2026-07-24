#!/usr/bin/env python3
from pathlib import Path
import re, sys
p=Path(sys.argv[1])
t=p.read_text(encoding="utf-8")
names=sorted(set(re.findall(r'VENOM_TJS_PUBLIC\("([A-Za-z0-9_]+)"\)', t)))
for n in names: print(n)

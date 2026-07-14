#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(__file__).resolve().parents[2]
runner=(root/'tools/run_release_closure.py').read_text(encoding='utf-8')
ps=(root/'scripts/release-closure.ps1').read_text(encoding='utf-8')
sh=(root/'scripts/release-closure.sh').read_text(encoding='utf-8')
required=(
 'final_release_gate.py','release_closure.py','npm','hardener-import-probe','hardener-functional-probe','cmake','ctest',
 'hardener-import-probe','hardener-functional-probe','doctor-production','protected-chess','nova-trade','bot-detection','analyze-dist','check-production-leaks.py',
 'package_release.py','release-closure-report.json','RELEASE CLOSURE',
)
missing=[x for x in required if x not in runner]
if missing:
 print('release closure runner smoke: FAIL: missing '+', '.join(missing),file=sys.stderr); raise SystemExit(1)
if 'run_release_closure.py' not in ps or 'run_release_closure.py' not in sh:
 print('release closure runner smoke: FAIL: wrappers do not share canonical runner',file=sys.stderr); raise SystemExit(1)
print('release closure runner smoke: PASS')

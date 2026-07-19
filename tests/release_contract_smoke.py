#!/usr/bin/env python3
from pathlib import Path
import json, subprocess, sys, tempfile, re
ROOT=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as td:
    td=Path(td); old=td/'old.json'; cur=td/'cur.json'
    subprocess.run([sys.executable,str(ROOT/'tools/generate_contract_manifest.py'),'--header',str(ROOT/'include/venom/generated/contracts/product_contracts.hpp'),'--version','1.44.0','--out',str(old)],check=True)
    cmake=(ROOT/'CMakeLists.txt').read_text(); version=re.search(r'VERSION\s+(\d+\.\d+\.\d+)',cmake).group(1)
    subprocess.run([sys.executable,str(ROOT/'tools/generate_contract_manifest.py'),'--header',str(ROOT/'include/venom/generated/contracts/product_contracts.hpp'),'--version',version,'--out',str(cur)],check=True)
    subprocess.run([sys.executable,str(ROOT/'tools/check_contract_upgrade.py'),str(old),str(cur)],check=True)
    d=json.loads(cur.read_text())
    assert d['contracts']['public_bridge_api']==1
print('release contract smoke: PASS')

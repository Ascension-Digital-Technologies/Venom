#!/usr/bin/env python3
"""Report and gate Venom runtime artifact size and release ABI surface."""
from __future__ import annotations
import argparse, gzip, hashlib, json, re, sys
try:
    import brotli
except ImportError:
    brotli=None
from pathlib import Path

def sha256(p:Path): return hashlib.sha256(p.read_bytes()).hexdigest()
def embedded_wasm(root:Path):
    h=root/'src/compiler/quickjs_runtime_wasm_blob.hpp'
    values=bytearray(); inside=False
    with h.open('r',encoding='utf-8',errors='replace') as f:
        for line in f:
            if not inside:
                if 'kQuickJsRuntimeWasmBlob[]' in line: inside=True
                continue
            if '};' in line: break
            for token in line.split(','):
                token=token.strip()
                if token.startswith('0x'): values.append(int(token,16))
    if not values: raise RuntimeError('embedded QuickJS/WASM byte array not found')
    return bytes(values)

def dist_metrics(d:Path):
    files=[p for p in d.rglob('*') if p.is_file()]
    return {'path':str(d),'files':len(files),'total_bytes':sum(p.stat().st_size for p in files),
      'javascript_bytes':sum(p.stat().st_size for p in files if p.suffix=='.js'),
      'wasm_bytes':sum(p.stat().st_size for p in files if p.suffix=='.wasm'),
      'package_bytes':sum(p.stat().st_size for p in files if p.suffix=='.vbc')}

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--repo-root',type=Path,default=Path(__file__).resolve().parents[1]); ap.add_argument('--artifact',type=Path); ap.add_argument('--dist',type=Path); ap.add_argument('--budget',type=Path); ap.add_argument('--json-out',type=Path); ap.add_argument('--format',choices=['text','json'],default='text')
    a=ap.parse_args(); root=a.repo_root.resolve(); data=a.artifact.read_bytes() if a.artifact else embedded_wasm(root)
    abi_text=(root/'tools/quickjs_release_abi.py').read_text(encoding='utf-8')
    block=re.search(r'RELEASE_ABI\s*=\s*\[(.*?)\]',abi_text,re.S)
    exports=re.findall(r"['\"](venom_qjs_[A-Za-z0-9_]+)['\"]",block.group(1) if block else '')
    report={'schema':'venom.runtime-performance.v1','quickjs_wasm':{'raw_bytes':len(data),'gzip_bytes':len(gzip.compress(data,compresslevel=9,mtime=0)),'brotli_bytes':len(brotli.compress(data,quality=11)) if brotli else None,'sha256':hashlib.sha256(data).hexdigest(),'release_abi_exports':len(exports)},'distribution':dist_metrics(a.dist) if a.dist else None,'gates':[],'passed':True}
    budget=a.budget or root/'contracts/runtime-performance.json'
    if budget.exists():
      b=json.loads(budget.read_text())['budgets']; pairs=[('quickjs_wasm.raw_bytes','quickjs_wasm_raw_bytes'),('quickjs_wasm.gzip_bytes','quickjs_wasm_gzip_bytes'),('quickjs_wasm.brotli_bytes','quickjs_wasm_brotli_bytes'),('quickjs_wasm.release_abi_exports','release_abi_exports')]
      if report['distribution']:
        pairs += [('distribution.javascript_bytes','distribution_javascript_bytes'),('distribution.total_bytes','distribution_total_bytes')]
      for path,key in pairs:
        cur=report
        for part in path.split('.'): cur=cur[part]
        if cur is None:
            report['gates'].append({'metric':path,'actual':None,'maximum':b[key],'passed':True,'skipped':'brotli module unavailable'}); continue
        ok=cur<=b[key]; report['gates'].append({'metric':path,'actual':cur,'maximum':b[key],'passed':ok}); report['passed'] &= ok
    text=json.dumps(report,indent=2)
    if a.format=='json': print(text)
    else:
      q=report['quickjs_wasm']; print('Venom runtime performance report'); print(f"QuickJS/WASM raw: {q['raw_bytes']} bytes"); print(f"QuickJS/WASM gzip: {q['gzip_bytes']} bytes"); print(f"QuickJS/WASM brotli: {q['brotli_bytes'] if q['brotli_bytes'] is not None else 'unavailable'}"); print(f"Release ABI exports: {q['release_abi_exports']}")
      for g in report['gates']: print(f"[{'PASS' if g['passed'] else 'FAIL'}] {g['metric']}: {g['actual']} <= {g['maximum']}")
    if a.json_out: a.json_out.write_text(text+'\n')
    return 0 if report['passed'] else 60
if __name__=='__main__': raise SystemExit(main())

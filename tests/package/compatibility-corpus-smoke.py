#!/usr/bin/env python3
import pathlib, shutil, subprocess, sys
if len(sys.argv) != 4:
    raise SystemExit('usage: compatibility-corpus-smoke.py <venom> <site> <out-root>')
venom=pathlib.Path(sys.argv[1]).resolve(); site=pathlib.Path(sys.argv[2]).resolve(); out=pathlib.Path(sys.argv[3]).resolve()
if out.exists(): shutil.rmtree(out)
out.mkdir(parents=True)
debug=out/'dev'; protected=out/'protected'
subprocess.run([str(venom),'build',str(site),'--out',str(debug),'--profile','dev','--hashed'],check=True)
subprocess.run([str(venom),'build',str(site),'--out',str(protected),'--profile','prod','--hashed'],check=True)
subprocess.run([str(venom),'release-check',str(protected),'--target','browser'],check=True)
subprocess.run([str(venom),'verify-runtime',str(protected),'--target','browser','--require-real-engine'],check=True)
# Debug preserves route shells; protected collapses them into the package.
for rel in ['index.html','about.html','account/settings.html']:
    if not (debug/rel).is_file(): raise SystemExit(f'missing debug route shell: {rel}')
if sorted(p.name for p in protected.iterdir()) != ['assets','index.html']:
    raise SystemExit(f'protected root is not minimal: {[p.name for p in protected.iterdir()]}')
assets=list((protected/'assets').rglob('*'))
required=[r'app\.[0-9a-f]+\.vbc$',r's\.[0-9a-f]+\.css$',r'loader\.[0-9a-f]+\.js$',r'engine\.[0-9a-f]+\.js$',r'runtime\.[0-9a-f]+\.wasm$',r'worker\.[0-9a-f]+\.js$']
import re
names=[p.name for p in assets if p.is_file()]
for pattern in required:
    if not any(re.match(pattern,name) for name in names): raise SystemExit(f'missing protected artifact {pattern}: {names}')
# Ensure representative non-code assets survived collection.
manifest_candidates=list((debug/'assets').glob('asset-manifest.txt'))
if manifest_candidates:
    text=manifest_candidates[0].read_text(encoding='utf-8')
    for marker in ['config.json','logo.svg','pattern.svg']:
        if marker not in text: raise SystemExit(f'asset manifest missing {marker}')
print('v0.93.0 compatibility corpus smoke: PASS')

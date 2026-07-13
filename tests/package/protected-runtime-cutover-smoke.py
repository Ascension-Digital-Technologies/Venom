#!/usr/bin/env python3
import pathlib, shutil, subprocess, sys
venom=pathlib.Path(sys.argv[1]); site=pathlib.Path(sys.argv[2]); out=pathlib.Path(sys.argv[3])
if out.exists(): shutil.rmtree(out)
rejected=subprocess.run([str(venom),'build',str(site),'--out',str(out),'--profile','release','--allow-host-fallback'],text=True,capture_output=True)
if rejected.returncode == 0 or 'structurally fail-closed' not in rejected.stdout + rejected.stderr:
    raise SystemExit('release host fallback override was not rejected')
subprocess.run([str(venom),'build',str(site),'--out',str(out),'--profile','browser-protect','--runtime','wasm','--hashed','--strict-release'],check=True,text=True,capture_output=True)
text='\n'.join(p.read_text(encoding='utf-8') for p in (out/'assets').glob('*.js'))
for token in ['new Function','eval(','host-js-fallback','host-js-isolated-wrapper','legacy-host-js-wrapper','__venomDisabledSourceEval','__venomDisabledEval']:
    if token in text: raise SystemExit(f'protected artifact contains {token!r}')
print('protected runtime cutover smoke passed')

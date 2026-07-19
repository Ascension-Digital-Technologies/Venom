#!/usr/bin/env python3
from __future__ import annotations
import json, os, stat, subprocess, sys
from pathlib import Path

def main()->int:
    root=Path(sys.argv[1]).resolve(); out=Path(sys.argv[2]).resolve(); out.mkdir(parents=True,exist_ok=True)
    fake=out/('emcc.exe' if os.name=='nt' else 'emcc')
    fake.write_text('#!/usr/bin/env python3\nimport sys\nprint("emcc fake 1.0")\n',encoding='utf-8'); fake.chmod(fake.stat().st_mode|stat.S_IXUSR)
    tool=root/'tools/quickjs_runtime_lifecycle.py'
    cp=subprocess.run([sys.executable,str(tool),'resolve-emcc','--repo-root',str(root),'--emcc',str(fake),'--format','path'],text=True,stdout=subprocess.PIPE)
    if cp.returncode or Path(cp.stdout.strip()).resolve()!=fake.resolve(): return 1
    fallback_env=os.environ.copy(); fallback_env['EMCC']=str(fake)
    cp=subprocess.run([sys.executable,str(tool),'resolve-emcc','--repo-root',str(root),'--emcc','--format','path'],text=True,stdout=subprocess.PIPE,env=fallback_env)
    if cp.returncode or Path(cp.stdout.strip()).resolve()!=fake.resolve():
        print('bare --emcc should fall back to EMCC discovery')
        return 1
    cp=subprocess.run([sys.executable,str(tool),'status','--repo-root',str(root),'--out-dir',str(out/'runtime'),'--emcc',str(fake),'--format','json'],text=True,stdout=subprocess.PIPE)
    info=json.loads(cp.stdout)
    if info['emcc_source']!='explicit' or 'artifact-missing' not in info['reasons'] or info['embedded_real_engine'] is not True: return 1
    ps=(root/'tools'/'windows-scripts'/'build-quickjs-wasm.ps1').read_text(encoding='utf-8')
    sh=(root/'tools'/'linux-scripts'/'build-quickjs-wasm.sh').read_text(encoding='utf-8')
    for marker in ('quickjs_runtime_lifecycle.py','New-LifecycleArguments','$env:EMCC','$resolvedEmccPath','[switch]$Force','[switch]$Status'):
        if marker not in ps: print('missing ps marker',marker); return 1
    if '"--emcc", $Emcc' in ps or '--emcc $Emcc `' in ps:
        print('PowerShell build script must not pass an empty direct --emcc argument')
        return 1
    for marker in ('quickjs_runtime_lifecycle.py','--force','--status'):
        if marker not in sh: print('missing sh marker',marker); return 1
    print('quickjs runtime lifecycle smoke: PASS'); return 0
if __name__=='__main__': raise SystemExit(main())

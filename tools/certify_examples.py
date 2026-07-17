#!/usr/bin/env python3
"""Build and verify every Venom example through the production-only pipeline."""
from __future__ import annotations
import argparse, hashlib, json, os, shutil, subprocess, sys, time
from datetime import datetime, timezone
from pathlib import Path

def run(command:list[str], cwd:Path, timeout:int=600)->dict:
    started=time.perf_counter()
    try:
        cp=subprocess.run(command,cwd=cwd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=timeout)
        code=cp.returncode; output=cp.stdout
    except subprocess.TimeoutExpired as exc:
        code=124; output=(exc.stdout or '')+(exc.stderr or '')+'\nstep timed out'
    return {'command':command,'passed':code==0,'returnCode':code,'durationMs':round((time.perf_counter()-started)*1000),'output':output[-30000:]}

def sha(path:Path)->str:
    h=hashlib.sha256(); h.update(path.read_bytes()); return h.hexdigest()

def write_md(result:dict,path:Path):
    lines=['# Venom Example Certification','',f"**Result:** {'PASS' if result['passed'] else 'FAIL'}  ",f"**Generated:** {result['generatedAt']}  ",'','| # | Example | Build | Verify | Runtime | Leak scan | Duration |','|---:|---|---:|---:|---:|---:|---:|']
    for item in result['examples']:
        def st(key): return 'PASS' if item.get(key) else 'FAIL'
        lines.append(f"| {item['number']} | `{item['id']}` | {st('buildPassed')} | {st('verifyPassed')} | {st('runtimePassed')} | {st('leakScanPassed')} | {item['durationMs']/1000:.2f}s |")
    lines += ['','## Failures','']
    failed=[x for x in result['examples'] if not x['passed']]
    if not failed: lines.append('No failures.')
    for item in failed:
        lines += [f"### {item['id']}",'', '```text', item.get('failureOutput','')[-8000:], '```','']
    path.write_text('\n'.join(lines)+'\n',encoding='utf-8')

def main()->int:
    ap=argparse.ArgumentParser(); ap.add_argument('--repo-root',type=Path,default=Path(__file__).resolve().parents[1]); ap.add_argument('--venom',type=Path,required=True); ap.add_argument('--contract',type=Path,default=Path('contracts/examples.json')); ap.add_argument('--output-dir',type=Path,default=Path('build/example-certification')); ap.add_argument('--keep-dists',action='store_true'); a=ap.parse_args()
    root=a.repo_root.resolve(); venom=a.venom.resolve(); contract_path=a.contract if a.contract.is_absolute() else root/a.contract; out=a.output_dir if a.output_dir.is_absolute() else root/a.output_dir
    contract=json.loads(contract_path.read_text()); assert contract['schema']=='VENOM_EXAMPLE_CERTIFICATION_V1'; out.mkdir(parents=True,exist_ok=True); dist_root=out/'distributions'; dist_root.mkdir(exist_ok=True)
    env=os.environ.copy(); env.setdefault('SOURCE_DATE_EPOCH','1704067200')
    results=[]
    for i,spec in enumerate(contract['examples']):
        site=root/spec['path']; dist=dist_root/spec['id']; shutil.rmtree(dist,ignore_errors=True); started=time.perf_counter(); steps=[]
        build=run([str(venom),'build',str(site),'--out',str(dist),'--profile','prod','--seed',str(2000000+i)],root); steps.append(('build',build))
        verify={'passed':False,'output':'build failed'}; runtime={'passed':False,'output':'build failed'}; leak={'passed':False,'output':'build failed'}
        if build['passed']:
            verify=run([str(venom),'verify',str(dist),'--target','browser'],root); steps.append(('verify',verify))
            runtime=run([str(venom),'verify-runtime',str(dist),'--target','browser','--require-real-engine'],root); steps.append(('runtime',runtime))
            leak=run([sys.executable,str(root/'tools/check_production_leaks.py'),str(dist)],root); steps.append(('leak',leak))
        passed=all(x[1]['passed'] for x in steps)
        failure='\n\n'.join(f"[{name}]\n{step['output']}" for name,step in steps if not step['passed'])
        item={'id':spec['id'],'number':spec['number'],'path':spec['path'],'passed':passed,'buildPassed':build['passed'],'verifyPassed':verify['passed'],'runtimePassed':runtime['passed'],'leakScanPassed':leak['passed'],'durationMs':round((time.perf_counter()-started)*1000),'steps':{name:step for name,step in steps},'failureOutput':failure}
        results.append(item); (out/f"{spec['id']}.json").write_text(json.dumps(item,indent=2)+'\n')
        print(f"[{'PASS' if passed else 'FAIL'}] {spec['id']} ({item['durationMs']/1000:.2f}s)",flush=True)
    result={'schema':'VENOM_EXAMPLE_CERTIFICATION_RESULT_V1','generatedAt':datetime.now(timezone.utc).isoformat(),'passed':all(x['passed'] for x in results),'profile':'prod','contract':{'path':str(contract_path.relative_to(root)),'sha256':sha(contract_path)},'examples':results}
    (out/'summary.json').write_text(json.dumps(result,indent=2)+'\n'); write_md(result,out/'summary.md')
    if not a.keep_dists: shutil.rmtree(dist_root,ignore_errors=True)
    return 0 if result['passed'] else 1
if __name__=='__main__': raise SystemExit(main())

#!/usr/bin/env python3
"""Securely check, install, and roll back Venom platform releases."""
from __future__ import annotations
import argparse, hashlib, json, os, platform, shutil, subprocess, sys, tempfile, urllib.parse, urllib.request
from pathlib import Path
from typing import Any

PRODUCT='Venom - Secure Web Runtime'; SCHEMA=1

def default_prefix()->Path:
    if os.name=='nt': return Path(os.environ.get('LOCALAPPDATA',Path.home()/'AppData'/'Local'))/'Venom'
    return Path(os.environ.get('XDG_DATA_HOME',Path.home()/'.local'/'share'))/'venom'

def target_triplet()->str:
    machine=platform.machine().lower(); arch='arm64' if machine in ('arm64','aarch64') else 'x64'
    system=platform.system().lower()
    return ('windows' if system=='windows' else 'macos' if system=='darwin' else 'linux')+'-'+arch

def sha256(path:Path)->str:
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024),b''): h.update(chunk)
    return h.hexdigest()

def fetch(source:str,dest:Path)->Path:
    parsed=urllib.parse.urlparse(source)
    if parsed.scheme in ('http','https'):
        if parsed.scheme!='https': raise SystemExit('update URLs must use HTTPS')
        req=urllib.request.Request(source,headers={'User-Agent':'Venom-Updater/1'})
        with urllib.request.urlopen(req,timeout=60) as r, dest.open('wb') as o:
            shutil.copyfileobj(r,o)
        return dest
    if parsed.scheme=='file': src=Path(urllib.request.url2pathname(parsed.path))
    else: src=Path(source)
    if not src.is_file(): raise SystemExit(f'update source not found: {source}')
    shutil.copy2(src,dest); return dest

def load_json_source(source:str,work:Path)->tuple[dict[str,Any],Path]:
    path=fetch(source,work/'RELEASE_SET.json')
    data=json.loads(path.read_text(encoding='utf-8'))
    if data.get('schema_version')!=1 or data.get('product')!=PRODUCT: raise SystemExit('release-set manifest is not recognized')
    return data,path

def semver(v:str)->tuple:
    main,_,pre=v.partition('-'); parts=main.split('.')
    if len(parts)!=3 or not all(x.isdigit() for x in parts): raise SystemExit(f'invalid release version: {v}')
    return tuple(map(int,parts))+(0 if not pre else -1,pre)

def receipt(prefix:Path)->dict[str,Any]|None:
    p=prefix/'install.json'
    return json.loads(p.read_text(encoding='utf-8')) if p.is_file() else None

def choose_package(manifest:dict[str,Any],triplet:str)->dict[str,Any]:
    matches=[p for p in manifest.get('packages',[]) if triplet in p.get('name','')]
    if len(matches)!=1: raise SystemExit(f'release set does not contain exactly one package for {triplet}')
    return matches[0]

def base_source(manifest_source:str,name:str)->str:
    parsed=urllib.parse.urlparse(manifest_source)
    if parsed.scheme in ('http','https'): return urllib.parse.urljoin(manifest_source,name if name.startswith('artifacts/') else 'artifacts/'+name)
    p=Path(urllib.request.url2pathname(parsed.path)) if parsed.scheme=='file' else Path(manifest_source)
    base=p.parent
    candidate=base/'artifacts'/name
    if not candidate.exists(): candidate=base/name
    return str(candidate)

def verify_signature(manifest_path:Path, source:str, public_key:Path|None, required:bool, root:Path)->None:
    sig_source=source.rsplit('/',1)[0]+'/RELEASE_SET.sig' if source.startswith('https://') else str(Path(source).parent/'RELEASE_SET.sig')
    parsed=urllib.parse.urlparse(sig_source)
    if parsed.scheme not in ('http','https'):
        local=Path(urllib.request.url2pathname(parsed.path)) if parsed.scheme=='file' else Path(sig_source)
        if not local.is_file():
            if required: raise SystemExit('signed update metadata is required but RELEASE_SET.sig is unavailable')
            return
    try:
        fetch(sig_source,root/'RELEASE_SET.sig')
    except (OSError, urllib.error.URLError):
        if required: raise SystemExit('signed update metadata is required but RELEASE_SET.sig is unavailable')
        return
    if not public_key:
        if required: raise SystemExit('--public-key is required for signed update metadata')
        return
    helper=Path(__file__).with_name('verify_release_set.py')
    cmd=[sys.executable,str(helper),str(manifest_path.parent),'--public-key',str(public_key),'--strict-signature']
    r=subprocess.run(cmd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    if r.returncode: sys.stdout.write(r.stdout); raise SystemExit('release-set signature verification failed')

def installed_contracts(prefix:Path)->Path|None:
    r=receipt(prefix)
    if not r:return None
    p=Path(r['release_root'])/'CONTRACTS.json'; return p if p.is_file() else None

def inspect_archive_contracts(archive:Path,work:Path)->Path|None:
    import zipfile, tarfile
    out=work/'contracts'; out.mkdir()
    if archive.suffix.lower()=='.zip':
        with zipfile.ZipFile(archive) as z:
            names=[n for n in z.namelist() if n.endswith('/CONTRACTS.json') or n=='CONTRACTS.json']
            if len(names)!=1:return None
            (out/'CONTRACTS.json').write_bytes(z.read(names[0]))
    elif archive.name.endswith(('.tar.gz','.tgz')):
        with tarfile.open(archive,'r:gz') as t:
            members=[m for m in t.getmembers() if m.name.endswith('/CONTRACTS.json') or m.name=='CONTRACTS.json']
            if len(members)!=1:return None
            f=t.extractfile(members[0]); (out/'CONTRACTS.json').write_bytes(f.read())
    return out/'CONTRACTS.json'

def check_contracts(current:Path|None,new:Path|None)->None:
    if not current or not new:return
    tool=Path(__file__).with_name('check_contract_upgrade.py')
    r=subprocess.run([sys.executable,str(tool),str(current),str(new)],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    if r.returncode: sys.stdout.write(r.stdout); raise SystemExit('update violates stable contract compatibility')

def emit(data:dict[str,Any],fmt:str)->None:
    if fmt=='json': print(json.dumps(data,indent=2,sort_keys=True)); return
    for k,v in data.items(): print(f'{k}: {v}')

def main()->int:
    ap=argparse.ArgumentParser(description=__doc__); ap.add_argument('action',choices=['check','install','rollback','status'])
    ap.add_argument('--channel',choices=['stable','preview'],default='stable'); ap.add_argument('--manifest',default=os.environ.get('VENOM_UPDATE_MANIFEST',''))
    ap.add_argument('--prefix',type=Path,default=default_prefix()); ap.add_argument('--public-key',type=Path); ap.add_argument('--require-signature',action='store_true')
    ap.add_argument('--yes',action='store_true'); ap.add_argument('--dry-run',action='store_true'); ap.add_argument('--format',choices=['text','json'],default='text')
    a=ap.parse_args(); prefix=a.prefix.expanduser().resolve(); rec=receipt(prefix)
    if a.action=='status': emit({'installed':bool(rec),'version':rec.get('version') if rec else None,'channel':(rec or {}).get('channel','unknown'),'healthy':bool(rec and Path(rec['command']).is_file())},a.format); return 0
    if a.action=='rollback':
        history=prefix/'update-history.json'; data=json.loads(history.read_text()) if history.is_file() else {'entries':[]}
        candidates=[x for x in data.get('entries',[]) if Path(x.get('release_root','')).is_dir()]
        if not candidates: raise SystemExit('no previous release is available for rollback')
        target=candidates[-1]; command=Path(rec['command']) if rec else prefix/'bin'/('venom.exe' if os.name=='nt' else 'venom')
        binary=Path(target['release_root'])/'bin'/command.name
        if not binary.is_file(): raise SystemExit('rollback release is missing its executable')
        tmp=command.with_suffix(command.suffix+'.rollback'); shutil.copy2(binary,tmp); os.replace(tmp,command)
        (prefix/'install.json').write_text(json.dumps(target,indent=2,sort_keys=True)+'\n'); emit({'rolled_back':True,'version':target['version']},a.format); return 0
    if not a.manifest: raise SystemExit('no update manifest configured; use --manifest or VENOM_UPDATE_MANIFEST')
    with tempfile.TemporaryDirectory(prefix='venom-update-') as td:
        work=Path(td); manifest,mfile=load_json_source(a.manifest,work)
        version=str(manifest.get('version','')); preview='-' in version
        if a.channel=='stable' and preview: raise SystemExit('stable channel rejected a preview release')
        verify_signature(mfile,a.manifest,a.public_key,a.require_signature,work)
        pkg=choose_package(manifest,target_triplet()); source=base_source(a.manifest,pkg['name']); archive=fetch(source,work/pkg['name'])
        if sha256(archive)!=pkg.get('sha256'): raise SystemExit('update package SHA-256 mismatch')
        current_version=rec.get('version') if rec else None; available=not current_version or semver(version)>semver(current_version)
        new_contracts=inspect_archive_contracts(archive,work); check_contracts(installed_contracts(prefix),new_contracts)
        result={'channel':a.channel,'current':current_version,'available':version,'update_available':available,'target':target_triplet(),'package_sha256':pkg['sha256']}
        if a.action=='check' or a.dry_run: emit(result,a.format); return 0
        if not available: emit({**result,'installed':False,'reason':'already current or newer'},a.format); return 0
        if not a.yes and sys.stdin.isatty():
            if input(f'Install Venom {version}? [y/N] ').strip().lower() not in ('y','yes'): raise SystemExit('update cancelled')
        installer=Path(__file__).with_name('install_release.py'); verifier=Path(__file__).with_name('verify_release.py')
        before=rec
        cmd=[sys.executable,str(installer),'install',str(archive),'--prefix',str(prefix),'--verifier',str(verifier),'--force']
        if a.require_signature: cmd.append('--require-signature')
        if a.public_key: cmd += ['--public-key',str(a.public_key)]
        r=subprocess.run(cmd,text=True); 
        if r.returncode:return r.returncode
        installed=receipt(prefix); installed['channel']=a.channel; (prefix/'install.json').write_text(json.dumps(installed,indent=2,sort_keys=True)+'\n')
        if before:
            hp=prefix/'update-history.json'; hist=json.loads(hp.read_text()) if hp.is_file() else {'schema_version':1,'entries':[]}; hist['entries'].append(before); hp.write_text(json.dumps(hist,indent=2,sort_keys=True)+'\n')
        emit({**result,'installed':True},a.format); return 0
if __name__=='__main__': raise SystemExit(main())

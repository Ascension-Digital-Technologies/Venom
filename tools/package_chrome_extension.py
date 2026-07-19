#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json, shutil, subprocess, sys, zipfile
from pathlib import Path

def main():
    ap=argparse.ArgumentParser(description='Create a Chrome Web Store-ready ZIP from a verified Venom extension distribution.')
    ap.add_argument('dist'); ap.add_argument('--out-dir',default='release/chrome-extension'); ap.add_argument('--name')
    ns=ap.parse_args(); root=Path(ns.dist).resolve(); out=Path(ns.out_dir).resolve(); out.mkdir(parents=True,exist_ok=True)
    manifest=json.loads((root/'manifest.json').read_text('utf-8')); stem=ns.name or f"{manifest.get('name','extension').lower().replace(' ','-')}-{manifest.get('version','0')}"
    report=out/f'{stem}-store-readiness.json'
    verify=Path(__file__).with_name('verify_chrome_extension_store.py')
    rc=subprocess.run([sys.executable,str(verify),str(root),'--report',str(report)]).returncode
    if rc: return rc
    unpacked=out/f'{stem}-unpacked'
    if unpacked.exists(): shutil.rmtree(unpacked)
    shutil.copytree(root,unpacked,ignore=shutil.ignore_patterns('chrome-store-readiness.json'))
    archive=out/f'{stem}.zip'
    with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for p in sorted(unpacked.rglob('*')):
            if p.is_file(): z.write(p,p.relative_to(unpacked).as_posix())
    digest=hashlib.sha256(archive.read_bytes()).hexdigest()
    (out/'SHA256SUMS.txt').write_text(f'{digest}  {archive.name}\n','utf-8')
    summary={'schema':'venom-chrome-extension-release-v1','archive':archive.name,'sha256':digest,'unpacked':unpacked.name,'readinessReport':report.name}
    (out/f'{stem}-build-report.json').write_text(json.dumps(summary,indent=2)+'\n','utf-8')
    print(f'Chrome extension package: {archive}\nSHA-256: {digest}')
    return 0
if __name__=='__main__': raise SystemExit(main())

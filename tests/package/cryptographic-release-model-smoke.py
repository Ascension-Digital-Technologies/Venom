#!/usr/bin/env python3
from __future__ import annotations
import shutil, subprocess, sys
from pathlib import Path

def run(cmd, expect=0):
    r=subprocess.run(cmd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=120)
    if r.returncode != expect:
        print(r.stdout); raise SystemExit(f'expected {expect}, got {r.returncode}: {cmd}')
    return r

def project_version(root: Path) -> str:
    import re
    text=(root/'CMakeLists.txt').read_text(encoding='utf-8')
    match=re.search(r'project\(venom VERSION ([0-9]+\.[0-9]+\.[0-9]+)',text)
    if not match: raise RuntimeError('unable to read project version')
    return match.group(1)

def main():
    if len(sys.argv)!=4: return 2
    root=Path(sys.argv[1]).resolve(); venom=Path(sys.argv[2]).resolve(); out=Path(sys.argv[3]).resolve(); version_value=project_version(root)
    if out.exists(): shutil.rmtree(out)
    keys=out.parent/'v090-keys'; shutil.rmtree(keys,ignore_errors=True); keys.mkdir()
    priv=keys/'private.pem'; pub=keys/'public.pem'
    run(['openssl','genpkey','-algorithm','ED25519','-out',str(priv)])
    run(['openssl','pkey','-in',str(priv),'-pubout','-out',str(pub)])
    run([sys.executable,str(root/'tools/package_release.py'),'--repo-root',str(root),'--venom',str(venom),'--out',str(out),'--sign','ed25519','--private-key',str(priv),'--public-key',str(pub),'--release-sequence','90','--release-channel','stable'])
    verifier=root/'tools/verify_release.py'
    run([sys.executable,str(verifier),str(out),'--strict-signature','--public-key',str(pub),'--min-release-sequence','90','--expect-channel','stable','--expect-version',version_value])
    rollback=run([sys.executable,str(verifier),str(out),'--strict-signature','--public-key',str(pub),'--min-release-sequence','91'],expect=1)
    if 'rollback rejected' not in rollback.stdout: raise SystemExit('rollback floor was not enforced')
    wrong_priv=keys/'wrong-private.pem'; wrong_pub=keys/'wrong-public.pem'
    run(['openssl','genpkey','-algorithm','ED25519','-out',str(wrong_priv)])
    run(['openssl','pkey','-in',str(wrong_priv),'-pubout','-out',str(wrong_pub)])
    wrong=run([sys.executable,str(verifier),str(out),'--strict-signature','--public-key',str(wrong_pub)],expect=1)
    if 'fingerprint mismatch' not in wrong.stdout: raise SystemExit('wrong trusted key was not rejected')
    print('cryptographic release model smoke: PASS'); return 0
if __name__=='__main__': raise SystemExit(main())

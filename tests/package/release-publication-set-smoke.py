#!/usr/bin/env python3
from __future__ import annotations
import hashlib, json, subprocess, sys, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]

def run(*args, expect=0):
    p=subprocess.run([sys.executable,*map(str,args)],text=True,capture_output=True)
    if p.returncode!=expect: raise SystemExit(f'command failed ({p.returncode} != {expect}): {p.args}\n{p.stdout}\n{p.stderr}')
    return p
with tempfile.TemporaryDirectory() as td:
    t=Path(td); (t/'linux.zip').write_bytes(b'linux'); (t/'windows.zip').write_bytes(b'windows'); (t/'evidence.zip').write_bytes(b'evidence')
    out=t/'set'
    run(ROOT/'tools/package_release_set.py','--version','1.12.0','--source-revision','abc123','--packages',t/'linux.zip',t/'windows.zip','--compatibility-evidence',t/'evidence.zip','--out',out,'--archive',t/'set.zip','--source-date-epoch','1704067200')
    run(ROOT/'tools/verify_release_set.py',out,'--require-evidence','--expect-version','1.12.0')
    obj=json.loads((out/'RELEASE_SET.json').read_text())
    assert obj['version']=='1.12.0' and len(obj['packages'])==2 and obj['compatibility_evidence']['name']=='evidence.zip'
    first=hashlib.sha256((out/'RELEASE_SET.json').read_bytes()).hexdigest()
    out2=t/'set2'
    run(ROOT/'tools/package_release_set.py','--version','1.12.0','--source-revision','abc123','--packages',t/'linux.zip',t/'windows.zip','--compatibility-evidence',t/'evidence.zip','--out',out2,'--archive',t/'set2.zip','--source-date-epoch','1704067200')
    assert first==hashlib.sha256((out2/'RELEASE_SET.json').read_bytes()).hexdigest()
    assert hashlib.sha256((t/'set.zip').read_bytes()).hexdigest()==hashlib.sha256((t/'set2.zip').read_bytes()).hexdigest()
    run(ROOT/'tools/verify_release_set.py',t/'set.zip','--require-evidence','--expect-version','1.12.0')
    run(ROOT/'tools/package_release_set.py','--version','1.12.0','--source-revision','abc123','--packages',t/'linux.zip','--out',t/'unsigned-required','--source-date-epoch','1704067200','--require-signature',expect=1)
    (out/'artifacts/linux.zip').write_bytes(b'tampered')
    run(ROOT/'tools/verify_release_set.py',out,'--require-evidence',expect=60)
print('release publication set smoke: PASS')

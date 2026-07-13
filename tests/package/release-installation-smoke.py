#!/usr/bin/env python3
from __future__ import annotations
import json, os, stat, subprocess, sys, tempfile, zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
TOOL=ROOT/'tools/install_release.py'

def run(*args, expect=0):
    p=subprocess.run([sys.executable,*map(str,args)],text=True,capture_output=True)
    if p.returncode!=expect:
        raise SystemExit(f'command failed ({p.returncode} != {expect}): {p.args}\n{p.stdout}\n{p.stderr}')
    return p

with tempfile.TemporaryDirectory() as td:
    t=Path(td); release=t/'venom-release'; release.mkdir()
    binary=release/('venom.exe' if os.name=='nt' else 'venom')
    if os.name=='nt':
        binary.write_bytes(b'MZ-fake')
    else:
        binary.write_text('#!/bin/sh\necho venom 1.13.0\n',encoding='utf-8')
        binary.chmod(binary.stat().st_mode|stat.S_IXUSR)
    (release/'VERSION.txt').write_text('venom 1.13.0\n',encoding='utf-8')
    (release/'RELEASE_MANIFEST.txt').write_text('fixture\n',encoding='utf-8')
    verifier=t/'verify.py'; verifier.write_text('import sys\nraise SystemExit(0)\n',encoding='utf-8')
    archive=t/'release.zip'
    with zipfile.ZipFile(archive,'w') as z:
        for p in release.rglob('*'):
            z.write(p,(Path(release.name)/p.relative_to(release)).as_posix())
    prefix=t/'prefix'; bindir=t/'bin'
    result=run(TOOL,'install',archive,'--prefix',prefix,'--bin-dir',bindir,'--verifier',verifier,'--format','json')
    installed=json.loads(result.stdout)
    assert installed['version']=='1.13.0'
    assert Path(installed['release_root']).is_dir()
    assert Path(installed['command']).is_file()
    status=json.loads(run(TOOL,'status','--prefix',prefix,'--format','json').stdout)
    assert status['healthy'] is True
    run(TOOL,'install',archive,'--prefix',prefix,'--bin-dir',bindir,'--verifier',verifier,expect=1)
    run(TOOL,'install',archive,'--prefix',prefix,'--bin-dir',bindir,'--verifier',verifier,'--force')
    run(TOOL,'uninstall','--prefix',prefix)
    assert not (prefix/'install.json').exists()
    assert not (bindir/('venom.exe' if os.name=='nt' else 'venom')).exists()

    unsafe=t/'unsafe.zip'
    with zipfile.ZipFile(unsafe,'w') as z: z.writestr('../escape.txt',b'no')
    run(TOOL,'install',unsafe,'--prefix',prefix,'--bin-dir',bindir,'--verifier',verifier,expect=1)
    assert not (t/'escape.txt').exists()
print('release installation smoke: PASS')

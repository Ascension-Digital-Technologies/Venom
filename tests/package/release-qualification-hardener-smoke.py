#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
workflow = (root / '.github/workflows/release-qualification.yml').read_text(encoding='utf-8')

linux_setup = './scripts/setup-js-hardener.sh'
windows_setup = r'.\scripts\setup-js-hardener.ps1 -NoPause'
qualification = 'tools/release_qualification.py'

assert workflow.count(linux_setup) == 1, 'Linux qualification job must install the pinned JS hardener exactly once'
assert workflow.count(windows_setup) == 1, 'Windows qualification job must install the pinned JS hardener exactly once'
assert workflow.count(qualification) == 2, 'Both qualification jobs must invoke release_qualification.py'

linux_job = workflow.split('qualify-linux:', 1)[1].split('qualify-windows:', 1)[0]
windows_job = workflow.split('qualify-windows:', 1)[1]
assert linux_job.index(linux_setup) < linux_job.index(qualification), 'Linux hardener setup must precede qualification'
assert windows_job.index(windows_setup) < windows_job.index(qualification), 'Windows hardener setup must precede qualification'
assert 'setup-node@v4' in linux_job and 'setup-node@v4' in windows_job
assert 'node-version: \'22\'' in workflow
print('release qualification hardener workflow smoke: PASS')

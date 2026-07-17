#!/usr/bin/env python3
from __future__ import annotations
import importlib.util
import pathlib
import tempfile
from unittest import mock

ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location('venom_dev_server', ROOT / 'tools' / 'dev_server.py')
assert SPEC and SPEC.loader
mod = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(mod)

with tempfile.TemporaryDirectory() as td:
    root = pathlib.Path(td)
    site = root / 'site'; site.mkdir()
    dist = root / 'dist'; dist.mkdir()
    venom = root / 'venom'; venom.write_bytes(b'compiler-v1')
    source = site / 'main.js'; source.write_text('let a=1;', encoding='utf-8')

    first = mod.file_manifest(site)
    original_stat = source.stat()
    source.write_text('let b=2;', encoding='utf-8')  # same byte length
    import os
    os.utime(source, ns=(original_stat.st_atime_ns, original_stat.st_mtime_ns))
    second = mod.file_manifest(site)
    assert first != second, 'content digest must detect same-size, same-mtime changes'

    (dist / 'index.html').write_text('last-good', encoding='utf-8')

    def successful_run(cmd, **kwargs):
        out = pathlib.Path(cmd[cmd.index('--out') + 1])
        out.mkdir(parents=True, exist_ok=True)
        (out / 'index.html').write_text('new-good', encoding='utf-8')
        return type('Result', (), {'returncode': 0, 'stdout': 'ok\n'})()

    with mock.patch.object(mod.subprocess, 'run', successful_run):
        ok, _ = mod.run_build(venom, site, dist, second)
    assert ok and (dist / 'index.html').read_text(encoding='utf-8') == 'new-good'
    assert mod.cache_matches(venom, second, dist)

    def failed_run(cmd, **kwargs):
        out = pathlib.Path(cmd[cmd.index('--out') + 1])
        out.mkdir(parents=True, exist_ok=True)
        (out / 'index.html').write_text('partial-bad', encoding='utf-8')
        return type('Result', (), {'returncode': 1, 'stdout': 'expected failure\n'})()

    with mock.patch.object(mod.subprocess, 'run', failed_run):
        ok, _ = mod.run_build(venom, site, dist, second)
    assert not ok
    assert (dist / 'index.html').read_text(encoding='utf-8') == 'new-good', 'failed rebuild replaced last-good output'

    state = mod.State()
    state.success(17, ['main.js'])
    snap = state.snapshot()
    assert snap['durationMs'] == 17 and snap['changed'] == ['main.js'] and not snap['error']

print('dev server incremental smoke: PASS')

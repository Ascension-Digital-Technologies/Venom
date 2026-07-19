#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import io
import stat
import tarfile
import tempfile
import zipfile
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
spec = importlib.util.spec_from_file_location('verify_release', ROOT / 'tools' / 'verify_release.py')
module = importlib.util.module_from_spec(spec)
assert spec and spec.loader
sys.modules[spec.name] = module
spec.loader.exec_module(module)

DIGEST = '0' * 64

def expect_value_error(text: str) -> None:
    try:
        module.parse_manifest(text)
    except ValueError:
        return
    raise AssertionError('unsafe manifest was accepted')

base = 'VENOM_RELEASE_MANIFEST_V1\nfiles:\n'
expect_value_error(base + f'{DIGEST} 0 ..\\escape.exe\n')
expect_value_error(base + f'{DIGEST} 0 C:/escape.exe\n')
expect_value_error(base + f'{DIGEST} -1 bin/venom.exe\n')
expect_value_error(base + f'{DIGEST} 0 bin/venom.exe\n{DIGEST} 0 bin/venom.exe\n')
expect_value_error(base + f'{DIGEST} 0 bin/evil\tname\n')

with tempfile.TemporaryDirectory(prefix='venom-release-archive-safety-') as td:
    td = Path(td)
    zpath = td / 'unsafe.zip'
    with zipfile.ZipFile(zpath, 'w') as zf:
        info = zipfile.ZipInfo('release/link')
        info.create_system = 3
        info.external_attr = (stat.S_IFLNK | 0o777) << 16
        zf.writestr(info, '../../outside')
    (td / 'zip-out').mkdir()
    try:
        module.extract_archive(zpath, td / 'zip-out')
    except SystemExit:
        pass
    else:
        raise AssertionError('ZIP symbolic link was accepted')

    tpath = td / 'unsafe.tar.gz'
    with tarfile.open(tpath, 'w:gz') as tf:
        info = tarfile.TarInfo('release/link')
        info.type = tarfile.SYMTYPE
        info.linkname = '../../outside'
        tf.addfile(info)
    (td / 'tar-out').mkdir()
    try:
        module.extract_archive(tpath, td / 'tar-out')
    except SystemExit:
        pass
    else:
        raise AssertionError('TAR symbolic link was accepted')

print('release archive and manifest safety smoke: PASS')

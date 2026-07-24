#!/usr/bin/env python3
from __future__ import annotations
import json, subprocess, sys, tempfile
from pathlib import Path
repo=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='venom-phase7-') as td:
    out=Path(td)/'release'; (out/'bin').mkdir(parents=True); (out/'runtime').mkdir()
    binary=out/'bin'/'venom'; binary.write_bytes(b'venom-test-binary')
    (out/'runtime'/'turbojs-runtime.wasm').write_bytes(b'wasm-test')
    (out/'CONTRACTS.json').write_text('{}\n')
    (out/'toolchains.lock.json').write_text((repo/'toolchains.lock.json').read_text())
    subprocess.run([sys.executable,str(repo/'tools/generate_release_metadata.py'),'--repo-root',str(repo),'--release-root',str(out),'--version','1.5.0','--source-date-epoch','1704067200','--release-sequence','100','--release-channel','stable','--target-triplet','linux-x64'],check=True)
    sbom=json.loads((out/'SBOM.cdx.json').read_text())
    prov=json.loads((out/'PROVENANCE.intoto.json').read_text())
    policy=json.loads((out/'RELEASE_POLICY.json').read_text())
    assert sbom['metadata']['component']['version']=='1.5.0'
    assert len(sbom['components']) >= 2
    assert all(c.get('hashes') for c in sbom['components'])
    subjects={s['name'] for s in prov['subject']}
    assert 'bin/venom' in subjects and 'runtime/turbojs-runtime.wasm' in subjects and 'CONTRACTS.json' in subjects
    assert policy['schema']=='VENOM_RELEASE_POLICY_V1'
    assert policy['security']['hostSourceFallbackAllowed'] is False
    assert policy['targetTriplet']=='linux-x64'
text=(repo/'tools/sign_release.py').read_text()
assert 'a.prod.resolve()' in text and 'a.release.resolve()' not in text
print('release engineering smoke: PASS')

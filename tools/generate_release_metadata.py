#!/usr/bin/env python3
"""Generate deterministic CycloneDX SBOM and SLSA-inspired provenance metadata."""
from __future__ import annotations
import argparse, hashlib, json, os, platform, subprocess
from datetime import datetime, timezone
from pathlib import Path


def sha256(path: Path) -> str:
    h=hashlib.sha256()
    with path.open('rb') as f:
        for c in iter(lambda:f.read(1024*1024), b''): h.update(c)
    return h.hexdigest()

def epoch_time(epoch: int) -> str:
    return datetime.fromtimestamp(epoch, timezone.utc).replace(microsecond=0).isoformat().replace('+00:00','Z')

def command_version(cmd: list[str]) -> str:
    try:
        r=subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=10)
        return (r.stdout or '').splitlines()[0][:300]
    except Exception:
        return 'unavailable'

def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument('--repo-root', type=Path, required=True)
    ap.add_argument('--release-root', type=Path, required=True)
    ap.add_argument('--version', required=True)
    ap.add_argument('--source-date-epoch', type=int, required=True)
    ap.add_argument('--release-sequence', type=int, required=True)
    ap.add_argument('--release-channel', required=True)
    args=ap.parse_args()
    repo=args.repo_root.resolve(); out=args.release_root.resolve(); ts=epoch_time(args.source_date_epoch)
    components=[]
    qjs=repo/'third_party/quickjs'
    if qjs.exists():
        components.append({'type':'library','name':'QuickJS','version':'vendored','licenses':[{'license':{'name':'MIT'}}], 'properties':[{'name':'venom:path','value':'third_party/quickjs'}]})
    sodium=repo/'third_party/libsodium'
    if sodium.exists(): components.append({'type':'library','name':'libsodium','version':'vendored','properties':[{'name':'venom:path','value':'third_party/libsodium'}]})
    sbom={
      'bomFormat':'CycloneDX','specVersion':'1.5','serialNumber':f'urn:uuid:venom-{args.version}-{args.release_sequence}', 'version':1,
      'metadata':{'timestamp':ts,'component':{'type':'application','name':'venom','version':args.version}},
      'components':components
    }
    (out/'SBOM.cdx.json').write_text(json.dumps(sbom, sort_keys=True, separators=(',',':'))+'\n', encoding='utf-8')
    materials=[]
    for rel in ('CMakeLists.txt','toolchains.lock.json'):
        p=repo/rel
        if p.is_file(): materials.append({'uri':rel,'digest':{'sha256':sha256(p)}})
    prov={
      '_type':'https://in-toto.io/Statement/v1','subject':[{'name':'venom','digest':{'sha256':sha256(out/('venom.exe' if (out/'venom.exe').exists() else 'venom'))}}],
      'predicateType':'https://slsa.dev/provenance/v1',
      'predicate':{
        'buildDefinition':{'buildType':'https://venom.dev/build/release/v1','externalParameters':{'version':args.version,'release_sequence':args.release_sequence,'release_channel':args.release_channel},'internalParameters':{'source_date_epoch':args.source_date_epoch},'resolvedDependencies':materials},
        'runDetails':{'builder':{'id':'venom-local-release-builder-v1'},'metadata':{'invocationId':f'venom-{args.version}-{args.release_sequence}','startedOn':ts,'finishedOn':ts},'byproducts':[{'name':'toolchain','content':{'platform':platform.platform(),'python':platform.python_version(),'cmake':command_version(['cmake','--version']),'compiler_cxx':os.environ.get('CXX','default')}}]}
      }
    }
    (out/'PROVENANCE.intoto.json').write_text(json.dumps(prov, sort_keys=True, separators=(',',':'))+'\n', encoding='utf-8')
    return 0
if __name__=='__main__': raise SystemExit(main())

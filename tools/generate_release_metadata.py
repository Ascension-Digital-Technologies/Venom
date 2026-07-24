#!/usr/bin/env python3
"""Generate deterministic CycloneDX SBOM, SLSA provenance, and release policy."""
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

def git_revision(repo: Path) -> str:
    explicit=os.environ.get('GITHUB_SHA') or os.environ.get('VENOM_SOURCE_REVISION')
    if explicit: return explicit
    try:
        r=subprocess.run(['git','-C',str(repo),'rev-parse','HEAD'],text=True,capture_output=True,timeout=10)
        if r.returncode==0 and r.stdout.strip(): return r.stdout.strip()
    except Exception: pass
    return 'unknown'

def license_name(directory: Path) -> str:
    candidates=[]
    for pattern in ('LICENSE*','COPYING*','NOTICE*'):
        candidates.extend(sorted(directory.glob(pattern)))
    if not candidates: return 'NOASSERTION'
    text=candidates[0].read_text(encoding='utf-8',errors='ignore')[:5000].lower()
    if 'mit license' in text or 'permission is hereby granted' in text: return 'MIT'
    if 'apache license' in text and 'version 2.0' in text: return 'Apache-2.0'
    if 'bsd' in text and 'redistribution and use' in text: return 'BSD'
    return 'LicenseRef-Vendored'

def directory_digest(directory: Path) -> str:
    h=hashlib.sha256()
    for p in sorted(x for x in directory.rglob('*') if x.is_file() and '__pycache__' not in x.parts):
        rel=p.relative_to(directory).as_posix().encode()
        h.update(len(rel).to_bytes(4,'big')); h.update(rel)
        digest=bytes.fromhex(sha256(p)); h.update(digest)
    return h.hexdigest()

def critical_subjects(out: Path) -> list[dict]:
    names=['bin/venom','bin/venom.exe','runtime/turbojs-runtime.wasm','CONTRACTS.json','toolchains.lock.json']
    subjects=[]
    for rel in names:
        p=out/rel
        if p.is_file(): subjects.append({'name':rel,'digest':{'sha256':sha256(p)}})
    return subjects

def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument('--repo-root', type=Path, required=True)
    ap.add_argument('--release-root', type=Path, required=True)
    ap.add_argument('--version', required=True)
    ap.add_argument('--source-date-epoch', type=int, required=True)
    ap.add_argument('--release-sequence', type=int, required=True)
    ap.add_argument('--release-channel', required=True)
    ap.add_argument('--target-triplet', default='unknown')
    args=ap.parse_args()
    repo=args.repo_root.resolve(); out=args.release_root.resolve(); ts=epoch_time(args.source_date_epoch)
    components=[]
    third_party=repo/'third_party'
    if third_party.is_dir():
        for d in sorted(x for x in third_party.iterdir() if x.is_dir()):
            components.append({
              'type':'library','name':d.name,'version':'vendored',
              'bom-ref':f'vendored:{d.name}:{directory_digest(d)[:16]}',
              'hashes':[{'alg':'SHA-256','content':directory_digest(d)}],
              'licenses':[{'license':{'name':license_name(d)}}],
              'properties':[{'name':'venom:path','value':d.relative_to(repo).as_posix()}]
            })
    binary=(out/'bin'/'venom.exe') if (out/'bin'/'venom.exe').is_file() else (out/'bin'/'venom')
    app_hash=sha256(binary)
    sbom={
      'bomFormat':'CycloneDX','specVersion':'1.5',
      'serialNumber':f'urn:uuid:venom-{args.version}-{args.release_sequence}-{args.target_triplet}', 'version':1,
      'metadata':{'timestamp':ts,'component':{'type':'application','name':'venom','version':args.version,
        'hashes':[{'alg':'SHA-256','content':app_hash}],
        'properties':[{'name':'venom:target-triplet','value':args.target_triplet},{'name':'venom:release-channel','value':args.release_channel}]}},
      'components':components
    }
    (out/'SBOM.cdx.json').write_text(json.dumps(sbom, sort_keys=True, separators=(',',':'))+'\n', encoding='utf-8')
    materials=[]
    for rel in ('CMakeLists.txt','toolchains.lock.json','contracts/turbojs-wasm-abi.json','contracts/runtime-api.json'):
        p=repo/rel
        if p.is_file(): materials.append({'uri':rel,'digest':{'sha256':sha256(p)}})
    revision=git_revision(repo)
    prov={
      '_type':'https://in-toto.io/Statement/v1','subject':critical_subjects(out),
      'predicateType':'https://slsa.dev/provenance/v1',
      'predicate':{
        'buildDefinition':{'buildType':'https://venom.dev/build/release/v2','externalParameters':{
          'version':args.version,'release_sequence':args.release_sequence,'release_channel':args.release_channel,
          'target_triplet':args.target_triplet,'source_revision':revision},
          'internalParameters':{'source_date_epoch':args.source_date_epoch},'resolvedDependencies':materials},
        'runDetails':{'builder':{'id':os.environ.get('VENOM_BUILDER_ID','venom-local-release-builder-v2')},
          'metadata':{'invocationId':f'venom-{args.version}-{args.release_sequence}-{args.target_triplet}','startedOn':ts,'finishedOn':ts},
          'byproducts':[{'name':'toolchain','content':{'platform':platform.platform(),'python':platform.python_version(),
            'cmake':command_version(['cmake','--version']),'compiler_c':os.environ.get('CC','default'),'compiler_cxx':os.environ.get('CXX','default')}}]}
      }
    }
    (out/'PROVENANCE.intoto.json').write_text(json.dumps(prov, sort_keys=True, separators=(',',':'))+'\n', encoding='utf-8')
    abi={}
    abi_path=repo/'contracts'/'turbojs-wasm-abi.json'
    if abi_path.is_file():
        raw=json.loads(abi_path.read_text(encoding='utf-8'))
        abi={'runtimeAbi':raw.get('runtimeAbi') or raw.get('version'),'bridgeAbi':raw.get('bridgeAbi'),'bytecodeFormat':raw.get('bytecodeFormat')}
    runtime_api={}
    runtime_api_path=repo/'contracts'/'runtime-api.json'
    if runtime_api_path.is_file():
        raw=json.loads(runtime_api_path.read_text(encoding='utf-8'))
        runtime_api={'schema':raw.get('schema'),'version':raw.get('version'),'package':raw.get('package'),'semver':raw.get('compatibility',{}).get('semver')}
    policy={
      'schema':'VENOM_RELEASE_POLICY_V1','version':args.version,'releaseSequence':args.release_sequence,
      'channel':args.release_channel,'targetTriplet':args.target_triplet,'sourceDateEpoch':args.source_date_epoch,
      'sourceRevision':revision,'turbojsWasm':abi,'runtimeApi':runtime_api,
      'security':{'hostSourceFallbackAllowed':False,'signedManifestRequiredForStable':True,'supplyChainMetadataRequiredForStable':True},
      'supportedHosts':['windows-x64','linux-x64','linux-arm64','macos-arm64']
    }
    (out/'RELEASE_POLICY.json').write_text(json.dumps(policy,sort_keys=True,separators=(',',':'))+'\n',encoding='utf-8')
    return 0
if __name__=='__main__': raise SystemExit(main())

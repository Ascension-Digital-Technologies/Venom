#!/usr/bin/env python3
"""Generate the final source-tree readiness report for Venom 2.0.0."""
from __future__ import annotations
import argparse, json, re, subprocess, sys
from datetime import datetime, timezone
from pathlib import Path

from venom_tools.examples import load_example_registry

SCHEMA='VENOM_FINAL_READINESS_RESULT_V1'

def run(name: str, command: list[str], root: Path) -> dict:
    p=subprocess.run(command,cwd=root,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    return {'id':name,'passed':p.returncode==0,'returnCode':p.returncode,'command':command,'output':p.stdout[-12000:]}

def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo-root',type=Path,default=Path(__file__).resolve().parents[1])
    ap.add_argument('--json-out',type=Path)
    ap.add_argument('--markdown-out',type=Path)
    a=ap.parse_args(); root=a.repo_root.resolve()
    contract_path=root/'contracts/final-release.json'
    contract=json.loads(contract_path.read_text(encoding='utf-8'))
    if contract.get('schema')!='VENOM_FINAL_RELEASE_CONTRACT_V1': raise SystemExit('invalid final release contract')
    cmake=(root/'CMakeLists.txt').read_text(encoding='utf-8')
    m=re.search(r'VERSION\s+(\d+\.\d+\.\d+)',cmake)
    version=m.group(1) if m else 'unknown'
    checks=[]
    checks.append({'id':'version','passed':version==contract['version'],'detail':version})
    example_items=load_example_registry(root).certifiable()
    checks.append({'id':'example-count','passed':len(example_items)==contract['requiredExampleCount'],'detail':len(example_items)})
    browser=json.loads((root/'contracts/browser-certification.json').read_text(encoding='utf-8'))
    engines=browser.get('requiredBrowsers') or browser.get('browsers') or []
    if engines and isinstance(engines[0],dict): engines=[x.get('id') or x.get('name') for x in engines]
    checks.append({'id':'browser-engines','passed':set(contract['requiredBrowserEngines']).issubset(set(engines)),'detail':engines})
    missing_contracts=[x for x in contract['requiredContracts'] if not (root/'contracts'/x).is_file()]
    missing_tools=[x for x in contract['requiredTools'] if not (root/'tools'/x).is_file()]
    checks.append({'id':'required-contracts','passed':not missing_contracts,'detail':missing_contracts})
    checks.append({'id':'required-tools','passed':not missing_tools,'detail':missing_tools})
    searchable=[]
    for base in ('src','tools','packages','integrations','examples'):
        for p in (root/base).rglob('*'):
            if p.is_file() and p.suffix.lower() in {'.cpp','.c','.hpp','.h','.py','.js','.mjs','.ts','.tsx','.json','.md','.toml','.yml','.yaml'}:
                searchable.append(p)
    violations=[]
    for p in searchable:
        text=p.read_text(encoding='utf-8',errors='ignore')
        for pattern in contract['forbiddenSourcePatterns']:
            if pattern in text:
                # Explicit rejection messages/tests for the removed profile are allowed.
                if ('removed' in text or 'reject' in text or 'rejection' in text) and pattern in {'--profile dev','profile == \"dev\"','ProfileKind::Dev'}: continue
                violations.append({'file':p.relative_to(root).as_posix(),'pattern':pattern})
    checks.append({'id':'forbidden-patterns','passed':not violations,'detail':violations})
    commands=[
      ('documentation',[sys.executable,str(root/'tools/documentation_gate.py')]),
      ('source-layout',[sys.executable,str(root/'tests/package/source-layout-smoke.py'),str(root)]),
      ('contracts-layout',[sys.executable,str(root/'tests/package/specification-layout-smoke.py'),str(root)]),
      ('changelog-uniqueness',[sys.executable,str(root/'tests/package/changelog-uniqueness-smoke.py')]),
      ('examples-contract',[sys.executable,str(root/'tests/package/example-certification-contract-smoke.py')]),
      ('browser-contract',[sys.executable,str(root/'tests/package/browser-certification-v2-smoke.py')]),
      ('startup-observability',[sys.executable,str(root/'tests/package/runtime-startup-observability-smoke.py')]),
      ('startup-performance',[sys.executable,str(root/'tests/package/runtime-startup-performance-smoke.py')]),
      ('runtime-api',[sys.executable,str(root/'tests/package/runtime-api-contract-smoke.py')]),
      ('cxx17-portability',[sys.executable,str(root/'tests/package/cxx17-portability-smoke.py')]),
      ('production-profile',[sys.executable,str(root/'tests/runtime/production-only-profile-boundary-smoke.py')])
    ]
    executions=[run(n,c,root) for n,c in commands]
    passed=all(x['passed'] for x in checks) and all(x['passed'] for x in executions)
    result={'schema':SCHEMA,'generatedAt':datetime.now(timezone.utc).isoformat(),'passed':passed,'version':version,'contract':'contracts/final-release.json','checks':checks,'executions':executions,'claims':contract['claims']}
    payload=json.dumps(result,indent=2,sort_keys=True)+'\n'
    lines=['# Venom 2.0.0 Final Readiness','',f"**Result:** {'PASS' if passed else 'FAIL'}",'', '## Source and contract checks','', '| Check | Result |','|---|---:|']
    lines += [f"| `{x['id']}` | {'PASS' if x['passed'] else 'FAIL'} |" for x in checks]
    lines += ['', '## Executable gates','', '| Gate | Result |','|---|---:|']
    lines += [f"| `{x['id']}` | {'PASS' if x['passed'] else 'FAIL'} |" for x in executions]
    lines += ['', '## Release claims','']+[f'- {x}' for x in contract['claims']]+['']
    md='\n'.join(lines)
    if a.json_out: a.json_out.parent.mkdir(parents=True,exist_ok=True); a.json_out.write_text(payload,encoding='utf-8')
    if a.markdown_out: a.markdown_out.parent.mkdir(parents=True,exist_ok=True); a.markdown_out.write_text(md,encoding='utf-8')
    print(payload,end='')
    return 0 if passed else 1
if __name__=='__main__': raise SystemExit(main())

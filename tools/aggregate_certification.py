#!/usr/bin/env python3
"""Aggregate platform and browser evidence into one final Venom certification verdict."""
from __future__ import annotations
import argparse, hashlib, json
from datetime import datetime, timezone
from pathlib import Path

def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--contract', type=Path, required=True)
    ap.add_argument('--platform-report', type=Path, action='append', default=[])
    ap.add_argument('--browser-matrix', type=Path, action='append', default=[])
    ap.add_argument('--json-out', type=Path, required=True)
    ap.add_argument('--markdown-out', type=Path, required=True)
    a=ap.parse_args()
    contract=json.loads(a.contract.read_text(encoding='utf-8'))
    required_platforms=set(contract['requiredPlatforms']); required_browsers=set(contract['requiredBrowsers'])
    platforms={}; platform_sources=[]
    for path in a.platform_report:
        data=json.loads(path.read_text(encoding='utf-8'))
        pid=data.get('environment',{}).get('platformId','unknown')
        platforms[pid]=data.get('passed') is True
        platform_sources.append({'path':str(path),'sha256':digest(path),'platformId':pid,'passed':platforms[pid]})
    browsers={}; browser_sources=[]
    for path in a.browser_matrix:
        data=json.loads(path.read_text(encoding='utf-8'))
        for row in data.get('rows',[]):
            name=row.get('browser','unknown')
            browsers[name]=browsers.get(name,True) and row.get('passed') is True
        browser_sources.append({'path':str(path),'sha256':digest(path),'passed':data.get('passed') is True})
    missing_platforms=sorted(required_platforms-set(platforms)); missing_browsers=sorted(required_browsers-set(browsers))
    platform_pass=not missing_platforms and all(platforms.get(x) for x in required_platforms)
    browser_pass=not missing_browsers and all(browsers.get(x) for x in required_browsers)
    passed=platform_pass and browser_pass and all(x['passed'] for x in browser_sources)
    result={
      'schema':'VENOM_AGGREGATED_RELEASE_CERTIFICATION_V1','generatedAt':datetime.now(timezone.utc).isoformat(),
      'passed':passed,'contractSha256':digest(a.contract),'requiredPlatforms':sorted(required_platforms),
      'requiredBrowsers':sorted(required_browsers),'platforms':platforms,'browsers':browsers,
      'missingPlatforms':missing_platforms,'missingBrowsers':missing_browsers,
      'platformEvidence':platform_sources,'browserEvidence':browser_sources
    }
    a.json_out.parent.mkdir(parents=True,exist_ok=True); a.markdown_out.parent.mkdir(parents=True,exist_ok=True)
    a.json_out.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n',encoding='utf-8')
    lines=['# Aggregated Venom Release Certification','',f"**Result:** {'PASS' if passed else 'FAIL'}  ",'', '## Platforms','']
    for p in sorted(required_platforms): lines.append(f"- `{p}` — {'PASS' if platforms.get(p) else ('MISSING' if p not in platforms else 'FAIL')}")
    lines += ['', '## Browsers','']
    for b in sorted(required_browsers): lines.append(f"- `{b}` — {'PASS' if browsers.get(b) else ('MISSING' if b not in browsers else 'FAIL')}")
    lines += ['', '## Evidence integrity','']
    for source in platform_sources+browser_sources: lines.append(f"- `{source['sha256']}` — `{source['path']}`")
    a.markdown_out.write_text('\n'.join(lines)+'\n',encoding='utf-8')
    print(json.dumps(result,indent=2,sort_keys=True))
    return 0 if passed else 71
if __name__=='__main__': raise SystemExit(main())

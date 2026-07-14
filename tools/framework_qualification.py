#!/usr/bin/env python3
"""Validate and summarize framework compatibility fixtures."""
from __future__ import annotations
import argparse, json
from pathlib import Path

SCHEMA='venom.framework-qualification.v1'

def main()->int:
    ap=argparse.ArgumentParser()
    ap.add_argument('--repo-root',type=Path,default=Path(__file__).resolve().parents[1])
    ap.add_argument('--suite',type=Path,default=Path('tests/compatibility-suite.json'))
    ap.add_argument('--json-out',type=Path)
    ap.add_argument('--markdown-out',type=Path)
    args=ap.parse_args(); root=args.repo_root.resolve(); suite=(root/args.suite).resolve() if not args.suite.is_absolute() else args.suite
    data=json.loads(suite.read_text(encoding='utf-8')); rows=[]; failures=[]
    for entry in data.get('fixtures',[]):
        site=root/entry['site']; manifest_path=site/'venom.browser.json'
        if not manifest_path.is_file(): continue
        m=json.loads(manifest_path.read_text(encoding='utf-8')); fw=m.get('framework')
        if not fw: continue
        row={'id':entry['id'],'name':fw.get('name'),'version':fw.get('version'),'delivery':fw.get('delivery'),'support_tier':m.get('support_tier'),'evidence_level':m.get('evidence_level'),'claims':m.get('claims',[]),'scenario_count':len(m.get('scenarios',[]))}
        for key in ('name','version','delivery'):
            if not fw.get(key): failures.append(f"{entry['id']}: framework.{key} missing")
        if m.get('support_tier') not in ('probe','qualified-pattern','qualified'):
            failures.append(f"{entry['id']}: invalid support_tier")
        if not m.get('scenarios'): failures.append(f"{entry['id']}: no scenarios")
        rows.append(row)
    required={'React','Vue','Svelte'}; present={r['name'] for r in rows}
    for name in sorted(required-present): failures.append(f'missing required framework fixture: {name}')
    report={'schema':SCHEMA,'passed':not failures,'suite':str(suite.relative_to(root)),'frameworks':rows,'failures':failures}
    if args.json_out:
        p=args.json_out if args.json_out.is_absolute() else root/args.json_out; p.parent.mkdir(parents=True,exist_ok=True); p.write_text(json.dumps(report,indent=2)+'\n')
    lines=['# Framework qualification matrix','', '> Generated from checked-in browser-equivalence manifests. “Qualified pattern” validates representative production output patterns; it is not a blanket guarantee for every framework release or plugin.', '', '| Framework | Version/output pattern | Delivery | Tier | Evidence | Scenarios |','|---|---|---|---|---|---:|']
    for r in rows: lines.append(f"| {r['name']} | {r['version']} | {r['delivery']} | {r['support_tier']} | {r['evidence_level']} | {r['scenario_count']} |")
    lines += ['', '## Interpretation', '', '- **probe**: a focused compatibility signal.', '- **qualified-pattern**: source and protected builds are compared in real browsers for a representative production output pattern.', '- **qualified**: reserved for a pinned upstream application and broader scenario coverage.', '']
    if args.markdown_out:
        p=args.markdown_out if args.markdown_out.is_absolute() else root/args.markdown_out; p.parent.mkdir(parents=True,exist_ok=True); p.write_text('\n'.join(lines),encoding='utf-8')
    print(f"framework qualification: {'PASS' if not failures else 'FAIL'} ({len(rows)} fixtures)")
    for f in failures: print(f'  - {f}')
    return 0 if not failures else 1
if __name__=='__main__': raise SystemExit(main())

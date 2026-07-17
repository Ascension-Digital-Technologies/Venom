#!/usr/bin/env python3
from __future__ import annotations
import argparse, json
from pathlib import Path

TIERS={"probe":0,"candidate":1,"supported":2}

def main():
    ap=argparse.ArgumentParser(description='Aggregate Venom browser validation reports into a compatibility matrix and enforce support promotion rules.')
    ap.add_argument('reports', nargs='+', type=Path)
    ap.add_argument('--json-out', type=Path)
    ap.add_argument('--markdown-out', type=Path)
    ap.add_argument('--required-browsers', default='chromium,firefox,webkit')
    ap.add_argument('--min-checks', type=int, default=3)
    a=ap.parse_args(); required={x.strip() for x in a.required_browsers.split(',') if x.strip()}
    rows=[]; fixtures={}; all_pass=True
    for path in a.reports:
        d=json.loads(path.read_text(encoding='utf-8'))
        fid=d.get('fixture_id','unknown'); tier=d.get('support_tier','probe')
        f=fixtures.setdefault(fid,{'fixture':fid,'profile':d.get('fixture_profile','unspecified'),'evidence_level':d.get('evidence_level','behavioral'),'declared_support_tier':tier,'framework':d.get('framework'), 'claims':d.get('claims',[]),'browsers':{},'reports':[]})
        f['reports'].append(str(path))
        for result in d.get('results',[]):
            browser=result.get('browser','unknown'); passed=result.get('passed') is True; checks=len(result.get('checks',[])); all_pass &= passed
            f['browsers'][browser]={'passed':passed,'checks':checks}
            rows.append({'fixture':fid,'profile':f['profile'],'evidence_level':f['evidence_level'],'support_tier':tier,'framework':f['framework'],'claims':f['claims'],'browser':browser,'passed':passed,'checks':checks,'report':str(path)})
    promotions=[]; policy_ok=True
    for f in fixtures.values():
        covered=set(f['browsers'])
        passed_all=required.issubset(covered) and all(f['browsers'][b]['passed'] and f['browsers'][b]['checks']>=a.min_checks for b in required)
        eligible='supported' if passed_all and f['evidence_level'] in ('behavioral','conformance') else ('candidate' if any(v['passed'] for v in f['browsers'].values()) else 'probe')
        declared=f['declared_support_tier'] if f['declared_support_tier'] in TIERS else 'probe'
        valid=TIERS[declared] <= TIERS[eligible]
        policy_ok &= valid
        promotions.append({**f,'required_browsers':sorted(required),'eligible_support_tier':eligible,'promotion_policy_passed':valid,'missing_browsers':sorted(required-covered)})
    summary = {
        'fixture_count': len(promotions),
        'supported_count': sum(1 for f in promotions if f['eligible_support_tier'] == 'supported'),
        'candidate_count': sum(1 for f in promotions if f['eligible_support_tier'] == 'candidate'),
        'probe_count': sum(1 for f in promotions if f['eligible_support_tier'] == 'probe'),
        'browser_result_count': len(rows),
        'passed_browser_result_count': sum(1 for r in rows if r['passed']),
    }
    out={'schema_version':3,'passed':all_pass and policy_ok,'browser_results_passed':all_pass,'support_policy_passed':policy_ok,'required_browsers':sorted(required),'minimum_checks_per_browser':a.min_checks,'summary':summary,'rows':rows,'fixtures':promotions}
    text=json.dumps(out,indent=2,sort_keys=True)
    if a.json_out:
        a.json_out.parent.mkdir(parents=True, exist_ok=True)
        a.json_out.write_text(text+'\n',encoding='utf-8')
    if a.markdown_out:
        a.markdown_out.parent.mkdir(parents=True, exist_ok=True)
        lines = [
            '# Venom Compatibility Matrix', '',
            f"**Result:** {'PASS' if out['passed'] else 'FAIL'}  ",
            f"**Required browsers:** {', '.join(sorted(required))}  ",
            f"**Fixtures:** {summary['fixture_count']}  ", '',
            '| Fixture | Profile | Framework | Declared | Eligible | Chromium | Firefox | WebKit |',
            '|---|---|---|---:|---:|---:|---:|---:|',
        ]
        for f in sorted(promotions, key=lambda item: item['fixture']):
            framework = f.get('framework')
            if isinstance(framework, dict):
                framework = f"{framework.get('name','')} {framework.get('version','')}".strip()
            framework = framework or '—'
            def browser_cell(name):
                value = f['browsers'].get(name)
                if not value:
                    return 'missing'
                return ('PASS' if value['passed'] else 'FAIL') + f" ({value['checks']})"
            lines.append('| ' + ' | '.join([
                f['fixture'], f['profile'], str(framework), f['declared_support_tier'],
                f['eligible_support_tier'], browser_cell('chromium'), browser_cell('firefox'), browser_cell('webkit')
            ]) + ' |')
        lines += ['', '## Claims', '']
        for f in sorted(promotions, key=lambda item: item['fixture']):
            claims = ', '.join(f.get('claims') or []) or 'No explicit claims declared.'
            lines.append(f"- **{f['fixture']}** — {claims}")
        lines += ['', '> Compatibility evidence is behavioral and version-specific. It does not imply that every application built with a framework is automatically compatible.', '']
        a.markdown_out.write_text('\n'.join(lines), encoding='utf-8')
    print(text)
    return 0 if out['passed'] else 60
if __name__=='__main__': raise SystemExit(main())

#!/usr/bin/env python3
import json, subprocess, sys, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as td:
    d=Path(td); reports=[]
    for browser in ('chromium','firefox','webkit'):
        p=d/f'supported-{browser}.json'
        p.write_text(json.dumps({'fixture_id':'supported','fixture_profile':'static','evidence_level':'behavioral','support_tier':'supported','claims':['static-routes'],'results':[{'browser':browser,'passed':True,'checks':[{}, {}, {}]}]}),encoding='utf-8'); reports.append(str(p))
    probe=d/'probe.json'; probe.write_text(json.dumps({'fixture_id':'react-probe','fixture_profile':'framework','evidence_level':'behavioral','support_tier':'probe','framework':{'name':'React','version':'16'},'claims':['render'],'results':[{'browser':'chromium','passed':True,'checks':[{}, {}, {}]}]}),encoding='utf-8'); reports.append(str(probe))
    out=d/'matrix.json'; r=subprocess.run([sys.executable,str(root/'tools/compatibility_matrix.py'),*reports,'--json-out',str(out)],capture_output=True,text=True)
    assert r.returncode==0, r.stdout+r.stderr
    data=json.loads(out.read_text()); assert data['schema_version']==2 and data['passed']
    supported=next(x for x in data['fixtures'] if x['fixture']=='supported'); assert supported['eligible_support_tier']=='supported' and supported['promotion_policy_passed']
    probe_row=next(x for x in data['fixtures'] if x['fixture']=='react-probe'); assert probe_row['declared_support_tier']=='probe' and probe_row['missing_browsers']==['firefox','webkit']
    bad=d/'bad.json'; bad.write_text(json.dumps({'fixture_id':'bad','evidence_level':'behavioral','support_tier':'supported','results':[{'browser':'chromium','passed':True,'checks':[{}, {}, {}]}]}),encoding='utf-8')
    r=subprocess.run([sys.executable,str(root/'tools/compatibility_matrix.py'),str(bad)],capture_output=True,text=True); assert r.returncode==60
print('compatibility matrix smoke: ok')

#!/usr/bin/env python3
import json, subprocess, sys, tempfile
from pathlib import Path
root=Path(sys.argv[1]).resolve(); td=Path(tempfile.mkdtemp()); report=td/'browser.json'; out=td/'perf.json'
report.write_text(json.dumps({
 'schema_version':1,'fixture_id':'smoke','dist_sha256':'a'*64,'manifest_sha256':'b'*64,
 'results':[{'browser':'chromium','passed':True,'duration_ms':900,'performance':{'browser_launch_ms':100,'context_create_ms':20,'scenarios':[{'id':'home','navigation_wall_ms':120,'fixture_ready_ms':10,'action_ms':[5,7],'total_ms':160}]}}]
}))
cp=subprocess.run([sys.executable,str(root/'tools/browser_performance.py'),str(report),'--format','json','--json-out',str(out)],text=True,capture_output=True)
assert cp.returncode==0,(cp.stdout,cp.stderr)
r=json.loads(out.read_text()); assert r['schema']=='venom.browser-performance.v1' and r['passed'] is True
bad=json.loads(report.read_text()); bad['results'][0]['performance']['scenarios'][0]['navigation_wall_ms']=20000; report.write_text(json.dumps(bad))
cp=subprocess.run([sys.executable,str(root/'tools/browser_performance.py'),str(report)],text=True,capture_output=True)
assert cp.returncode==60,(cp.stdout,cp.stderr)
print('browser performance budget smoke passed')

#!/usr/bin/env python3
import json, subprocess, sys, tempfile
from pathlib import Path

if len(sys.argv) != 2:
    raise SystemExit('usage: capability-graph-smoke.py <venom>')
exe = Path(sys.argv[1])
with tempfile.TemporaryDirectory() as td:
    root = Path(td)
    (root/'index.html').write_text('''<!doctype html><script>\n// eval("ignored")\nconst text = "WebSocket require( process.env";\nimport("./chunk.js");\nnew MutationObserver(()=>{});\nReactDOM.render(React.createElement("div"), document.body);\n</script>''', encoding='utf-8')
    (root/'umd.js').write_text('(function(g,f){typeof exports==="object"&&typeof module!=="undefined"?module.exports=f(require("react")):g.X=f(g.React)})(this,function(React){return React});', encoding='utf-8')

    raw = subprocess.check_output([str(exe), 'analyze', str(root), '--format', 'json'], text=True)
    graph = json.loads(raw)
    assert graph['schema_version'] == 1
    assert graph['files_scanned'] == 2
    assert graph['module_features']['dynamic_import'] == 1
    assert graph['browser_apis']['MutationObserver'] == 1
    assert graph['frameworks']['React'] >= 2
    assert graph['node_apis']['umd_commonjs_branch'] == 1
    assert 'WebSocket' not in graph['browser_apis'], graph
    assert 'dynamic_eval' not in graph['syntax'], graph
    assert all(x['line'] > 0 and x['column'] > 0 for x in graph['occurrences'])

    result = subprocess.run([str(exe), 'compatibility', 'check', str(root), '--format', 'json'], text=True, capture_output=True)
    assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
    report = json.loads(result.stdout)
    assert report['schema_version'] == 2
    assert report['analysis'] == 'syntax-aware-capability-graph-v1'
    assert report['compatible'] is True
    assert report['capability_graph']['module_features']['dynamic_import'] == 1
    features = {x['feature']: x for x in report['findings']}
    assert features['javascript.dynamic-import']['status'] == 'partial'
    assert features['dom.mutation-observer']['status'] == 'partial'
    assert features['javascript.umd-commonjs-branch']['status'] == 'partial'

    (root/'bad.js').write_text('eval("x"); const value = require("node-only"); new WebSocket("/ws");', encoding='utf-8')
    result = subprocess.run([str(exe), 'compatibility', 'check', str(root), '--format', 'json'], text=True, capture_output=True)
    assert result.returncode == 20
    report = json.loads(result.stdout)
    features = {x['feature'] for x in report['findings']}
    assert {'javascript.eval', 'node.require', 'web.websocket'} <= features
print('capability graph smoke: ok')

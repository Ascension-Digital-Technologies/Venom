#!/usr/bin/env python3
import json, subprocess, sys, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]

def frame(obj):
    raw=json.dumps(obj,separators=(',',':')).encode(); return f'Content-Length: {len(raw)}\r\n\r\n'.encode()+raw
with tempfile.TemporaryDirectory() as td:
    p=Path(td)/'app.ts'; p.write_text('import x from "./missing";\n',encoding='utf-8')
    payload=b''.join([frame({'jsonrpc':'2.0','id':1,'method':'initialize','params':{}}),frame({'jsonrpc':'2.0','method':'textDocument/didOpen','params':{'textDocument':{'uri':p.as_uri(),'text':p.read_text()}}}),frame({'jsonrpc':'2.0','id':2,'method':'shutdown','params':{}}),frame({'jsonrpc':'2.0','method':'exit','params':{}})])
    cp=subprocess.run([sys.executable,str(root/'tools/venom_lsp.py')],input=payload,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=20)
    out=cp.stdout.decode(errors='replace')
    assert cp.returncode==0,cp.stderr.decode(); assert 'VENOM-D1001' in out; assert 'VENOM-D1101' in out; assert 'venom-lsp' in out
print('language server smoke passed')

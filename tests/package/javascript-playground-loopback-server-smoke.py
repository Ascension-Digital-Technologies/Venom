#!/usr/bin/env python3
from __future__ import annotations
import importlib.util, json, sys, tempfile, threading, urllib.error, urllib.request
from http.server import ThreadingHTTPServer
from pathlib import Path
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools'))
spec=importlib.util.spec_from_file_location('launcher',root/'tools/build_and_launch_example.py')
assert spec and spec.loader
mod=importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
with tempfile.TemporaryDirectory() as tmp:
    fake=Path(tmp)/'venom-fake.py'
    fake.write_text('#!/usr/bin/env python3\nimport base64,json,sys\nprint(json.dumps({"ok":True,"bytecode":base64.b64encode(sys.stdin.read().encode()).decode()}))\n')
    fake.chmod(0o755)
    class Bound(mod.QuietHandler): pass
    Bound.venom_executable=fake; Bound.playground_enabled=True; Bound.playground_token='secret-token'
    server=ThreadingHTTPServer(('127.0.0.1',0),lambda *a,**kw: Bound(*a,directory=tmp,**kw))
    thread=threading.Thread(target=server.serve_forever,daemon=True); thread.start()
    port=server.server_port; origin=f'http://127.0.0.1:{port}'
    try:
        session=urllib.request.Request(origin+'/__venom/playground/session',headers={'Referer':origin+'/'})
        payload=json.loads(urllib.request.urlopen(session,timeout=5).read())
        assert payload['token']=='secret-token' and payload['limits']['sourceBytes']==262144
        body=json.dumps({'source':'const message = "Unicode: … ✅ 東京"; message;'}, ensure_ascii=False).encode('utf-8')
        unauthorized=urllib.request.Request(origin+'/__venom/playground/compile',data=body,headers={'Content-Type':'application/json','Origin':origin},method='POST')
        try: urllib.request.urlopen(unauthorized,timeout=5); raise AssertionError('missing token accepted')
        except urllib.error.HTTPError as exc: assert exc.code==403
        cross=urllib.request.Request(origin+'/__venom/playground/compile',data=body,headers={'Content-Type':'application/json','Origin':'http://evil.invalid','X-Venom-Playground-Token':'secret-token'},method='POST')
        try: urllib.request.urlopen(cross,timeout=5); raise AssertionError('cross-origin request accepted')
        except urllib.error.HTTPError as exc: assert exc.code==403
        valid=urllib.request.Request(origin+'/__venom/playground/compile',data=body,headers={'Content-Type':'application/json','Origin':origin,'X-Venom-Playground-Token':'secret-token'},method='POST')
        result=json.loads(urllib.request.urlopen(valid,timeout=5).read())
        assert result['ok'] and result['bytecode']
        decoded=__import__('base64').b64decode(result['bytecode']).decode('utf-8')
        assert 'Unicode: … ✅ 東京' in decoded
    finally:
        server.shutdown(); server.server_close()
print('JavaScript playground loopback server: PASS')

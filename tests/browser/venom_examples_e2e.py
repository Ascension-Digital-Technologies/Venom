#!/usr/bin/env python3
from __future__ import annotations
import argparse, json, secrets, subprocess, threading, time
from datetime import datetime, timezone
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from pathlib import Path

class Handler(SimpleHTTPRequestHandler):
    venom: Path | None = None
    playground = False
    token = "venom-browser-certification-token"
    def log_message(self, *_): pass
    def _json(self,status,payload):
        body=json.dumps(payload,separators=(',',':')).encode('utf-8'); self.send_response(status); self.send_header('Content-Type','application/json; charset=utf-8'); self.send_header('Content-Length',str(len(body))); self.send_header('Cache-Control','no-store'); self.end_headers(); self.wfile.write(body)
    def do_GET(self):
        if self.playground and self.path == '/__venom/playground/session':
            self._json(200,{'ok':True,'token':self.token,'limits':{'sourceBytes':262144,'inputBytes':65536,'resultBytes':262144,'consoleEvents':128,'consoleBytes':131072,'executionMs':2000,'heapBytes':8388608,'stackBytes':262144}}); return
        super().do_GET()
    def do_POST(self):
        if not self.playground or self.path != '/__venom/playground/compile' or self.venom is None: self.send_error(404); return
        if not secrets.compare_digest(self.headers.get('X-Venom-Playground-Token',''),self.token): self._json(403,{'ok':False,'error':'token rejected'}); return
        try:
            length=int(self.headers.get('Content-Length','0')); request=json.loads(self.rfile.read(length).decode('utf-8'))
            proc=subprocess.run([str(self.venom),'compile-snippet'],input=str(request.get('source','')).encode('utf-8'),stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
            stdout=proc.stdout.decode('utf-8',errors='strict'); stderr=proc.stderr.decode('utf-8',errors='replace')
            body=(stdout if proc.returncode==0 else json.dumps({'ok':False,'error':(stderr or stdout).strip()})).encode('utf-8'); self.send_response(200 if proc.returncode==0 else 400)
        except Exception as exc: body=json.dumps({'ok':False,'error':str(exc)}).encode('utf-8'); self.send_response(500)
        self.send_header('Content-Type','application/json; charset=utf-8'); self.send_header('Content-Length',str(len(body))); self.send_header('Cache-Control','no-store'); self.end_headers(); self.wfile.write(body)

def serve(directory: Path, port: int, venom: Path, playground: bool):
    class Bound(Handler): pass
    Bound.venom=venom; Bound.playground=playground
    server=ThreadingHTTPServer(('127.0.0.1',port),lambda *a,**kw: Bound(*a,directory=str(directory),**kw)); threading.Thread(target=server.serve_forever,daemon=True).start(); return server

def qualify_page(browser, url, spec, timeout):
    page=browser.new_page(); errors=[]; responses=[]
    page.on('pageerror',lambda e: errors.append({'type':'pageerror','message':str(e)}))
    page.on('console',lambda m: errors.append({'type':'console','message':m.text}) if m.type=='error' else None)
    page.on('response',lambda r: responses.append({'url':r.url,'status':r.status}) if r.status >= 400 else None)
    started=time.perf_counter(); page.goto(url,wait_until='domcontentloaded',timeout=timeout)
    page.wait_for_function("globalThis.__venomBootStatus && ['ready','error'].includes(globalThis.__venomBootStatus.state)",timeout=timeout)
    status=page.evaluate("globalThis.__venomBootStatus")
    failure=page.locator('[data-venom-failure=true]')
    if status.get('state')!='ready': raise AssertionError(f"boot status: {status}")
    timeline=status.get('timeline') or []
    completed={entry.get('phase') for entry in timeline if entry.get('state')=='complete'}
    required_phases={'package-load','package-policy','runtime-install','route-decode','route-render','script-execution','navigation-install'}
    missing_phases=sorted(required_phases-completed)
    if missing_phases: raise AssertionError(f"boot timeline missing phases: {missing_phases}; status={status}")
    if any(float(entry.get('durationMs') or 0) < 0 for entry in timeline): raise AssertionError(f"negative boot phase duration: {timeline}")
    if failure.count(): raise AssertionError(failure.first.inner_text())
    page.locator(spec['readySelector']).first.wait_for(state='attached',timeout=30000)
    runtime=page.evaluate("globalThis.venom && typeof globalThis.venom.status==='function' ? globalThis.venom.status() : null")
    if runtime and not runtime.get('ready'): raise AssertionError(f"runtime not ready: {runtime}")
    checks=['boot-ready','boot-timeline','no-failure-panel','ready-selector','no-page-errors','no-http-errors']
    if spec.get('playground'):
        page.locator('#code-editor').fill('const city="東京"; ({ answer: 6 * 7, city, mark: "✅", ellipsis: "…" })'); page.locator('#run').click()
        page.wait_for_function("document.querySelector('#status')?.dataset.state === 'success'",timeout=30000)
        output=page.locator('#output').inner_text(); assert '42' in output and '東京' in output,output; checks.append('playground-utf8')
    if errors: raise AssertionError(f"browser errors: {errors}")
    if responses: raise AssertionError(f"HTTP errors: {responses}")
    result={'passed':True,'durationMs':round((time.perf_counter()-started)*1000),'bootStatus':status,'runtimeStatus':runtime,'checks':checks}; page.close(); return result

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--repo-root',type=Path,default=Path(__file__).resolve().parents[2]); ap.add_argument('--venom',type=Path,required=True); ap.add_argument('--browser',choices=['chromium','firefox','webkit','all'],default='chromium'); ap.add_argument('--contract',type=Path,default=Path('contracts/browser-certification.json')); ap.add_argument('--work-dir',type=Path); ap.add_argument('--json-out',type=Path); ap.add_argument('--keep-dists',action='store_true')
    a=ap.parse_args(); root=a.repo_root.resolve(); venom=a.venom.resolve(); contract_path=a.contract if a.contract.is_absolute() else root/a.contract; contract=json.loads(contract_path.read_text()); assert contract['schema']=='VENOM_BROWSER_CERTIFICATION_V2'
    work=(a.work_dir or root/'build/browser-certification').resolve(); work.mkdir(parents=True,exist_ok=True); built=[]
    for index,spec in enumerate(contract['examples']):
        out=work/spec['id']; subprocess.run([str(venom),'build',str(root/'examples'/spec['id']),'--out',str(out),'--profile','prod','--seed',str(2300000+index)],check=True,stdout=subprocess.DEVNULL)
        subprocess.run([str(venom),'verify-runtime',str(out),'--target','browser','--require-real-engine'],check=True,stdout=subprocess.DEVNULL); built.append((spec,out,18100+index))
    from playwright.sync_api import sync_playwright
    results=[]; browser_names=['chromium','firefox','webkit'] if a.browser=='all' else [a.browser]
    with sync_playwright() as p:
        for browser_name in browser_names:
            opts={'headless':True}
            if browser_name=='chromium' and Path('/usr/bin/chromium').is_file(): opts.update(executable_path='/usr/bin/chromium',args=['--no-sandbox'])
            browser=getattr(p,browser_name).launch(**opts)
            for spec,out,port in built:
                server=serve(out,port,venom,bool(spec.get('playground')))
                try:
                    try: detail=qualify_page(browser,f'http://127.0.0.1:{port}/',spec,int(contract['bootTimeoutMs'])); results.append({'browser':browser_name,'example':spec['id'],**detail})
                    except Exception as exc: results.append({'browser':browser_name,'example':spec['id'],'passed':False,'error':str(exc)})
                finally: server.shutdown(); server.server_close()
            browser.close()
    report={'schema':'VENOM_BROWSER_CERTIFICATION_RESULT_V2','generatedAt':datetime.now(timezone.utc).isoformat(),'passed':all(r['passed'] for r in results),'contract':str(contract_path.relative_to(root)),'results':results}
    text=json.dumps(report,indent=2); print(text)
    if a.json_out: a.json_out.parent.mkdir(parents=True,exist_ok=True); a.json_out.write_text(text+'\n',encoding='utf-8')
    if not a.keep_dists:
        import shutil
        for _,out,_ in built: shutil.rmtree(out,ignore_errors=True)
    return 0 if report['passed'] else 1
if __name__=='__main__': raise SystemExit(main())

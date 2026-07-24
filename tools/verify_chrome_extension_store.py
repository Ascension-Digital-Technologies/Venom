#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json, re, sys
from pathlib import Path, PurePosixPath
from urllib.parse import urlparse

JS_SUFFIXES={'.js','.mjs','.cjs'}
EXEC_KEYS={'service_worker','default_popup','devtools_page','options_page','page'}
REMOTE_EXEC_RE=re.compile(r"(?:importScripts\s*\(|(?:import|fetch)\s*\(|new\s+(?:Worker|SharedWorker)\s*\(|\.src\s*=)\s*[`\"']https?://",re.I)
INLINE_SCRIPT_RE=re.compile(r'<script\b(?![^>]*\bsrc\s*=)[^>]*>(.*?)</script\s*>',re.I|re.S)
SCRIPT_SRC_RE=re.compile(r'<script\b[^>]*\bsrc\s*=\s*([\"\'])(.*?)\1',re.I|re.S)
LINK_RE=re.compile(r'<(?:script|link)\b[^>]*(?:src|href)\s*=\s*([\"\'])(.*?)\1',re.I|re.S)
CHROME_RUNTIME_URL_RE=re.compile(r"\bchrome\.runtime\.getURL\s*\(\s*([`\"'])((?:\\.|(?!\1).)*)\1\s*\)",re.S)
DANGEROUS_JS=[re.compile(r'\beval\s*\('),re.compile(r'\bnew\s+Function\s*\('),re.compile(r'\bimport\s*\(\s*[`\"\']https?://',re.I)]
DEV_NAMES={'.git','.github','node_modules','tests','test','docs','examples','src','build','dist'}
DEV_FILES={'package.json','package-lock.json','yarn.lock','pnpm-lock.yaml','venom.toml','README.md','CHANGES.md','CONTRIBUTING.md'}

class Audit:
    def __init__(self, root:Path):
        self.root=root; self.errors=[]; self.warnings=[]; self.info=[]; self.refs=set(); self.hardened=[]
    def error(self,msg): self.errors.append(msg)
    def warn(self,msg): self.warnings.append(msg)
    def note(self,msg): self.info.append(msg)

def safe_rel(value:str)->str|None:
    value=value.replace('\\','/').split('?',1)[0].split('#',1)[0]
    p=PurePosixPath(value)
    if not value or p.is_absolute() or '..' in p.parts or re.match(r'^[A-Za-z]:',value): return None
    return str(p)

def add_ref(a:Audit,value,source):
    if not isinstance(value,str): return
    rel=safe_rel(value)
    if rel is None: a.error(f'unsafe resource path {value!r} referenced by {source}'); return
    a.refs.add(rel)

def walk_manifest(a:Audit,m:dict):
    if m.get('manifest_version')!=3: a.error('manifest_version must be 3')
    if not isinstance(m.get('name'),str) or not m['name'].strip(): a.error('manifest name is required')
    if not re.fullmatch(r'\d+(?:\.\d+){0,3}',str(m.get('version',''))): a.error('manifest version must contain one to four numeric components')
    action=m.get('action') or {}
    add_ref(a,action.get('default_popup'),'action.default_popup')
    for k,v in (action.get('default_icon') or {}).items() if isinstance(action.get('default_icon'),dict) else []: add_ref(a,v,f'action.default_icon.{k}')
    bg=m.get('background') or {}; add_ref(a,bg.get('service_worker'),'background.service_worker')
    for key in ('options_page','devtools_page'): add_ref(a,m.get(key),key)
    options=m.get('options_ui') or {}; add_ref(a,options.get('page'),'options_ui.page')
    side=m.get('side_panel') or {}; add_ref(a,side.get('default_path'),'side_panel.default_path')
    for k,v in (m.get('chrome_url_overrides') or {}).items(): add_ref(a,v,f'chrome_url_overrides.{k}')
    for i,cs in enumerate(m.get('content_scripts') or []):
        for kind in ('js','css'):
            for j,v in enumerate(cs.get(kind) or []): add_ref(a,v,f'content_scripts[{i}].{kind}[{j}]')
    for i,r in enumerate(m.get('declarative_net_request',{}).get('rule_resources',[]) or []): add_ref(a,r.get('path'),f'declarative_net_request.rule_resources[{i}]')
    icons=m.get('icons') or {}
    for k,v in icons.items(): add_ref(a,v,f'icons.{k}')
    for i,v in enumerate(m.get('sandbox',{}).get('pages',[]) or []): add_ref(a,v,f'sandbox.pages[{i}]')
    for i,entry in enumerate(m.get('web_accessible_resources') or []):
        for j,v in enumerate(entry.get('resources') or []):
            if '*' in v: a.warn(f'web-accessible wildcard: {v}')
            else: add_ref(a,v,f'web_accessible_resources[{i}].resources[{j}]')

def html_refs(a:Audit,path:Path):
    text=path.read_text('utf-8',errors='replace')
    for m in INLINE_SCRIPT_RE.finditer(text):
        if m.group(1).strip(): a.error(f'inline executable script in {path.relative_to(a.root)}')
    for _,ref in LINK_RE.findall(text):
        parsed=urlparse(ref)
        if parsed.scheme in {'http','https','data'} or ref.startswith('//'):
            if parsed.scheme in {'http','https'} or ref.startswith('//'): a.error(f'remote executable/resource URL in {path.relative_to(a.root)}: {ref}')
            continue
        rel=safe_rel(str((PurePosixPath(path.relative_to(a.root).as_posix()).parent / ref)))
        if rel: a.refs.add(rel)

def js_audit(a:Audit,path:Path):
    text=path.read_text('utf-8',errors='replace')
    rel=path.relative_to(a.root).as_posix()
    for rx in DANGEROUS_JS:
        if rx.search(text): a.error(f'forbidden dynamic code pattern in {rel}: {rx.pattern}')
    if REMOTE_EXEC_RE.search(text): a.error(f'remote executable loading primitive in JavaScript: {rel}')
    # chrome.runtime.getURL() resolves from the extension root, regardless of
    # which adapter contains the call. Record literal targets so missing static
    # resources fail store readiness instead of surfacing as a runtime fetch error.
    for match in CHROME_RUNTIME_URL_RE.finditer(text):
        target=match.group(2)
        if '${' in target:
            a.warn(f'dynamic chrome.runtime.getURL target cannot be verified statically in {rel}: {target}')
            continue
        add_ref(a,target,f'{rel}:chrome.runtime.getURL')
    # Hardened extension files should be compact and free of source comments. This is a conservative evidence check.
    if rel.startswith('assets/extension/') or '/' not in rel:
        if len(text)>400 and ('\n' in text and text.count('\n')>max(8,len(text)//500)): a.warn(f'JavaScript may not be fully compacted: {rel}')
        if re.search(r'(^|\n)\s*(//|/\*)',text): a.error(f'JavaScript contains source comments and may have bypassed hardening: {rel}')
        a.hardened.append(rel)

def main():
    ap=argparse.ArgumentParser(description='Verify a Venom Chrome extension distribution for Manifest V3 store readiness.')
    ap.add_argument('dist'); ap.add_argument('--report'); ap.add_argument('--strict-permissions',action='store_true')
    ns=ap.parse_args(); root=Path(ns.dist).resolve(); a=Audit(root)
    if not root.is_dir(): print(f'error: not a directory: {root}',file=sys.stderr); return 2
    manifest=root/'manifest.json'
    if not manifest.is_file(): a.error('manifest.json must be the only root file and is missing')
    root_entries=[p.name for p in root.iterdir()]
    try: m=json.loads(manifest.read_text('utf-8')) if manifest.is_file() else {}
    except Exception as e: a.error(f'invalid manifest.json: {e}'); m={}
    walk_manifest(a,m)
    allowed_root={'manifest.json','assets'}
    for ref in a.refs:
        path=PurePosixPath(ref)
        if len(path.parts)==1 and path.suffix.lower() in {'.html','.htm'}:
            allowed_root.add(path.name)
    # chrome.scripting.executeScript requires stable extension-relative file
    # paths. Root-level hardened adapters are valid Manifest V3 resources and
    # are audited below with the same dynamic-code and remote-load checks.
    for entry in root.iterdir():
        if entry.is_file() and entry.suffix.lower() in JS_SUFFIXES:
            allowed_root.add(entry.name)
    extras=sorted(x for x in root_entries if x not in allowed_root)
    if extras: a.error('unexpected root entries: '+', '.join(extras))
    csp=(m.get('content_security_policy') or {}).get('extension_pages','') if isinstance(m.get('content_security_policy'),dict) else str(m.get('content_security_policy',''))
    if "'unsafe-eval'" in csp: a.error("extension CSP must not contain 'unsafe-eval'")
    if "'wasm-unsafe-eval'" not in csp:
        a.error("extension CSP must contain 'wasm-unsafe-eval' for the protected TurboJS/WASM runtime")
    if "script-src 'self'" not in csp:
        a.error("extension CSP must restrict scripts to 'self'")
    if 'http:' in csp or 'https:' in csp or '*' in csp: a.error('extension CSP permits remote or wildcard sources')
    perms=list(m.get('permissions') or []); hosts=list(m.get('host_permissions') or [])
    high={'tabs','history','bookmarks','downloads','cookies','management','webRequest','webRequestBlocking','nativeMessaging','debugger','proxy'}
    broad=[p for p in perms if p in high]
    broad_hosts=[h for h in hosts if h in {'<all_urls>','*://*/*','http://*/*','https://*/*'}]
    if broad: (a.error if ns.strict_permissions else a.warn)('high-impact permissions: '+', '.join(broad))
    if broad_hosts: (a.error if ns.strict_permissions else a.warn)('broad host permissions: '+', '.join(broad_hosts))
    for p in sorted(root.rglob('*')):
        if not p.is_file(): continue
        rel=p.relative_to(root).as_posix()
        if any(part in DEV_NAMES for part in PurePosixPath(rel).parts) or p.name in DEV_FILES: a.error(f'development file included: {rel}')
        if p.suffix.lower() in {'.html','.htm'}: html_refs(a,p)
        if p.suffix.lower() in JS_SUFFIXES: js_audit(a,p)
    for rel in sorted(a.refs):
        if '*' not in rel and not (root/rel).is_file(): a.error(f'referenced resource is missing: {rel}')
    files=[]
    for p in sorted(root.rglob('*')):
        if p.is_file(): files.append({'path':p.relative_to(root).as_posix(),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
    report={'schema':'venom-chrome-store-readiness-v1','ok':not a.errors,'manifest':{'name':m.get('name'),'version':m.get('version'),'permissions':perms,'hostPermissions':hosts,'webAccessibleResources':len(m.get('web_accessible_resources') or [])},'layout':{'rootEntries':sorted(root_entries),'files':len(files),'javascriptFiles':len(a.hardened)},'errors':a.errors,'warnings':a.warnings,'files':files}
    out=Path(ns.report) if ns.report else root.parent/(root.name+'-store-readiness.json'); out.parent.mkdir(parents=True,exist_ok=True); out.write_text(json.dumps(report,indent=2)+'\n','utf-8')
    for x in a.errors: print(f'[FAIL] {x}')
    for x in a.warnings: print(f'[WARN] {x}')
    if a.errors: print(f'Chrome Web Store readiness: FAIL ({len(a.errors)} errors, report={out})'); return 1
    print(f'Chrome Web Store readiness: PASS ({len(files)} files, {len(a.hardened)} hardened JS, report={out})'); return 0
if __name__=='__main__': raise SystemExit(main())

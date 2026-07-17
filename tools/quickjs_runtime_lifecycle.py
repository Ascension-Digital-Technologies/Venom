#!/usr/bin/env python3
"""Resolve Emscripten and report/manage the QuickJS/WASM runtime lifecycle."""
from __future__ import annotations
import argparse, hashlib, json, os, re, shutil, subprocess, sys
from pathlib import Path

INPUTS = [
    'src/runtime/quickjs_runtime_scaffold.c',
    'third_party/quickjs/quickjs.c', 'third_party/quickjs/quickjs.h',
    'third_party/quickjs/quickjs-libc.c', 'third_party/quickjs/quickjs-libc.h',
    'third_party/quickjs/libregexp.c', 'third_party/quickjs/libregexp.h',
    'third_party/quickjs/libunicode.c', 'third_party/quickjs/libunicode.h',
    'third_party/quickjs/dtoa.c', 'third_party/quickjs/dtoa.h',
    'tools/quickjs_release_abi.py', 'tools/quickjs_wasm_cutover.py',
    'tools/wasm_exports.py', 'tools/embed_wasm.py',
]

def sha256_file(path: Path) -> str:
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024), b''): h.update(chunk)
    return h.hexdigest()

def executable(value: str | Path | None) -> Path | None:
    if not value: return None
    p=Path(str(value)).expanduser()
    if p.is_file(): return p.resolve()
    found=shutil.which(str(value))
    return Path(found).resolve() if found else None

def resolve_emcc(root: Path, explicit: str | None=None) -> tuple[Path|None,str]:
    candidates: list[tuple[str,str|Path|None]] = [
        ('explicit', explicit), ('EMCC', os.environ.get('EMCC')),
    ]
    emsdk=os.environ.get('EMSDK')
    if emsdk:
        candidates += [('EMSDK', Path(emsdk)/'upstream'/'emscripten'/('emcc.bat' if os.name=='nt' else 'emcc')),
                       ('EMSDK', Path(emsdk)/'upstream'/'emscripten'/('emcc.exe' if os.name=='nt' else 'emcc'))]
    base=root/'build'/'emsdk'/'upstream'/'emscripten'
    candidates += [('repository', base/('emcc.bat' if os.name=='nt' else 'emcc')),
                   ('repository', base/('emcc.exe' if os.name=='nt' else 'emcc')),
                   ('PATH','emcc')]
    for source,value in candidates:
        p=executable(value)
        if p: return p,source
    return None,'missing'

def resolve_wasm_opt(root: Path) -> tuple[Path|None,str]:
    emsdk=os.environ.get('EMSDK')
    candidates=[]
    if emsdk: candidates += [('EMSDK', Path(emsdk)/'upstream'/'bin'/('wasm-opt.exe' if os.name=='nt' else 'wasm-opt'))]
    candidates += [('repository', root/'build'/'emsdk'/'upstream'/'bin'/('wasm-opt.exe' if os.name=='nt' else 'wasm-opt')), ('PATH','wasm-opt')]
    for source,value in candidates:
        p=executable(value)
        if p: return p,source
    return None,'missing'

def emcc_version(emcc: Path|None) -> str:
    if not emcc: return ''
    try:
        cp=subprocess.run([str(emcc),'--version'],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=20)
        return cp.stdout.splitlines()[0].strip() if cp.stdout else ''
    except Exception: return ''

def input_digest(root: Path, emcc: Path|None, abi: str, opt: str, mem: str) -> str:
    h=hashlib.sha256()
    for rel in INPUTS:
        p=root/rel
        h.update(rel.encode()); h.update(b'\0')
        if p.is_file(): h.update(p.read_bytes())
        else: h.update(b'<missing>')
        h.update(b'\0')
    h.update(f'abi={abi}\nopt={opt}\nmem={mem}\nemcc={emcc_version(emcc)}\n'.encode())
    return h.hexdigest()

def embedded_info(root: Path) -> dict[str,str]:
    p=root/'src'/'generated'/'runtime'/'quickjs_runtime_wasm_blob.hpp'
    if not p.is_file(): return {}
    text=p.read_text(encoding='utf-8',errors='replace')
    m=re.search(r'kQuickJsRuntimeWasmBlobProvenance\s*=\s*R"[^\(]*\((.*?)\)[A-Za-z0-9_]*"',text,re.S)
    if not m: return {}
    out={}
    for line in m.group(1).splitlines():
        if '=' in line:
            k,v=line.split('=',1); out[k.strip()]=v.strip()
    return out

def status(root:Path,out:Path,emcc:Path|None,source:str,abi:str,opt:str,mem:str)->dict:
    wasm=out/'quickjs-runtime.wasm'; state_path=out/'quickjs-runtime.state.json'
    state={}
    if state_path.is_file():
        try: state=json.loads(state_path.read_text(encoding='utf-8'))
        except Exception: state={}
    digest=input_digest(root,emcc,abi,opt,mem)
    wasm_sha=sha256_file(wasm) if wasm.is_file() else ''
    embedded=embedded_info(root); embedded_sha=embedded.get('wasm_sha256','')
    reasons=[]
    if not emcc: reasons.append('emcc-not-found')
    if not wasm.is_file(): reasons.append('artifact-missing')
    if state.get('input_digest') != digest: reasons.append('build-inputs-changed')
    if wasm_sha and state.get('wasm_sha256') != wasm_sha: reasons.append('artifact-state-mismatch')
    current=not [r for r in reasons if r!='emcc-not-found']
    return {'schema_version':1,'emcc':str(emcc) if emcc else '', 'emcc_source':source,
      'emcc_version':emcc_version(emcc),'artifact':str(wasm),'artifact_exists':wasm.is_file(),
      'artifact_bytes':wasm.stat().st_size if wasm.is_file() else 0,'artifact_sha256':wasm_sha,
      'embedded_sha256':embedded_sha,'embedded_real_engine':embedded.get('full_upstream_quickjs')=='true',
      'artifact_matches_embedded':bool(wasm_sha and embedded_sha and wasm_sha==embedded_sha),
      'input_digest':digest,'current':current,'reasons':reasons,'abi_mode':abi,'optimization_profile':opt,'memory_profile':mem}

def main()->int:
    ap=argparse.ArgumentParser()
    ap.add_argument('command',choices=['resolve-emcc','resolve-wasm-opt','status','write-state'])
    ap.add_argument('--repo-root',type=Path,default=Path(__file__).resolve().parents[1])
    ap.add_argument('--out-dir',type=Path,default=None); ap.add_argument('--emcc', nargs='?', const='', default=None)
    ap.add_argument('--abi-mode',default='prod'); ap.add_argument('--optimization-profile',default='balanced'); ap.add_argument('--memory-profile',default='medium')
    ap.add_argument('--format',choices=['text','json','path'],default='text')
    args=ap.parse_args(); root=args.repo_root.resolve(); out=(args.out_dir or root/'build'/'quickjs-wasm').resolve()
    emcc,source=resolve_emcc(root,args.emcc)
    if args.command=='resolve-wasm-opt':
        tool,tool_source=resolve_wasm_opt(root)
        if not tool:
            print('[venom] wasm-opt was not found; Emscripten output will be used without Binaryen post-processing.',file=sys.stderr); return 3
        print(str(tool) if args.format=='path' else json.dumps({'wasm_opt':str(tool),'source':tool_source}))
        return 0
    if args.command=='resolve-emcc':
        if not emcc:
            print('[venom] emcc was not found. Run scripts/windows/build-emsdk.bat or pass -Emcc.',file=sys.stderr); return 3
        print(str(emcc) if args.format=='path' else json.dumps({'emcc':str(emcc),'source':source}))
        return 0
    info=status(root,out,emcc,source,args.abi_mode,args.optimization_profile,args.memory_profile)
    if args.command=='write-state':
        wasm=out/'quickjs-runtime.wasm'
        if not wasm.is_file(): print(f'[venom] artifact not found: {wasm}',file=sys.stderr); return 2
        state={'schema_version':1,'input_digest':info['input_digest'],'wasm_sha256':info['artifact_sha256'],
               'emcc':info['emcc'],'emcc_version':info['emcc_version'],'abi_mode':args.abi_mode,
               'optimization_profile':args.optimization_profile,'memory_profile':args.memory_profile}
        out.mkdir(parents=True,exist_ok=True); (out/'quickjs-runtime.state.json').write_text(json.dumps(state,indent=2)+'\n',encoding='utf-8')
        info=status(root,out,emcc,source,args.abi_mode,args.optimization_profile,args.memory_profile)
    if args.format=='json': print(json.dumps(info,indent=2))
    else:
        print('Venom QuickJS/WASM runtime status')
        for k in ('emcc','emcc_source','emcc_version','artifact','artifact_exists','artifact_bytes','artifact_sha256','embedded_sha256','embedded_real_engine','artifact_matches_embedded','current'):
            print(f'{k}: {info[k]}')
        if info['reasons']: print('reasons: '+', '.join(info['reasons']))
    return 0
if __name__=='__main__': raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations
import argparse, json, re, sys
from pathlib import Path

REQUIRED = [
    'README.md','CHANGES.md','LICENSE','NOTICE.md','SECURITY.md','SUPPORT.md',
    'docs/README.md','docs/getting-started/build-from-source.md','docs/operations/release-verification.md',
    'examples/protected-chess/README.md','examples/nova-trade/README.md','examples/bot-detection/README.md','.github/workflows/release.yml',
]
FORBIDDEN_DIRS = {'.git','build','dist','node_modules','__pycache__','.pytest_cache','.mypy_cache'}
FORBIDDEN_SUFFIXES = {'.pyc','.pyo','.key','.pem'}
FORBIDDEN_PUBLIC_EXAMPLES = {'basic-site','kitchen-sink','multipage','chess-ai'}


def cmake_version(root: Path) -> str:
    text=(root/'CMakeLists.txt').read_text(encoding='utf-8')
    m=re.search(r'project\s*\(\s*venom\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)',text,re.I|re.S)
    if not m: raise ValueError('unable to read project version from CMakeLists.txt')
    return m.group(1)


def markdown_links(path: Path):
    text=path.read_text(encoding='utf-8',errors='replace')
    for target in re.findall(r'\[[^\]]+\]\(([^)]+)\)',text):
        target=target.strip()
        if not target or '://' in target or target.startswith('mailto:'): continue
        yield target


def github_anchor(text: str) -> str:
    text=re.sub(r'<[^>]+>','',text).strip().lower()
    text=re.sub(r'[^\w\- ]','',text,flags=re.UNICODE)
    return re.sub(r'[ \t]+','-',text)


def markdown_anchors(path: Path) -> set[str]:
    anchors=set(); counts={}
    text=path.read_text(encoding='utf-8',errors='replace')
    for line in text.splitlines():
        m=re.match(r'^#{1,6}\s+(.+?)\s*#*$',line)
        if not m: continue
        base=github_anchor(m.group(1))
        if not base: continue
        count=counts.get(base,0)
        counts[base]=count+1
        anchors.add(base if count==0 else f'{base}-{count}')
    for name in re.findall(r'<a\s+(?:name|id)=["\']([^"\']+)["\']',text,re.I):
        anchors.add(name)
    return anchors


def main() -> int:
    ap=argparse.ArgumentParser(description='Validate GitHub/public-release repository hygiene.')
    ap.add_argument('repo',nargs='?',default='.')
    ap.add_argument('--json-out',type=Path)
    args=ap.parse_args()
    root=Path(args.repo).resolve()
    errors=[]; warnings=[]
    try: version=cmake_version(root)
    except Exception as exc: errors.append(str(exc)); version='unknown'

    for rel in REQUIRED:
        if not (root/rel).is_file(): errors.append(f'missing required release file: {rel}')

    readme=(root/'README.md').read_text(encoding='utf-8',errors='replace') if (root/'README.md').exists() else ''
    if version!='unknown' and not (f'**Version:** {version}' in readme or f'Version {version}</strong>' in readme):
        errors.append(f'README version does not match {version}')
    changes=(root/'CHANGES.md').read_text(encoding='utf-8',errors='replace') if (root/'CHANGES.md').exists() else ''
    if version!='unknown' and version not in changes:
        errors.append(f'CHANGES.md does not mention {version}')

    examples=root/'examples'
    public=[p.name for p in examples.iterdir() if p.is_dir()] if examples.exists() else []
    if sorted(public) != ['bot-detection', 'nova-trade', 'protected-chess']:
        errors.append(f'public examples must contain bot-detection, protected-chess and nova-trade; found {sorted(public)}')
    for name in FORBIDDEN_PUBLIC_EXAMPLES:
        if (examples/name).exists(): errors.append(f'legacy public example remains: examples/{name}')

    for p in root.rglob('*'):
        rel=p.relative_to(root)
        if any(part in FORBIDDEN_DIRS for part in rel.parts):
            if rel.parts[0] not in {'.git'}: errors.append(f'generated directory present: {rel}')
            continue
        if p.is_file() and p.suffix.lower() in FORBIDDEN_SUFFIXES:
            errors.append(f'forbidden sensitive/generated suffix: {rel}')

    for md in root.rglob('*.md'):
        if any(part in FORBIDDEN_DIRS for part in md.relative_to(root).parts): continue
        for link in markdown_links(md):
            file_part, sep, anchor = link.partition('#')
            target = md if not file_part else (md.parent/file_part).resolve()
            if not target.exists():
                errors.append(f'broken documentation link: {md.relative_to(root)} -> {file_part or link}')
                continue
            if sep and anchor and target.is_file() and target.suffix.lower()=='.md':
                if anchor not in markdown_anchors(target):
                    errors.append(f'broken documentation anchor: {md.relative_to(root)} -> {link}')

    license_text=(root/'LICENSE').read_text(encoding='utf-8',errors='replace') if (root/'LICENSE').exists() else ''
    if 'All rights reserved' in license_text:
        warnings.append('repository is source-available/restricted, not OSI open source; describe it accurately')
    if not (root/'third_party/quickjs/LICENSE').exists():
        errors.append('QuickJS license is missing')

    result={'version':version,'status':'PASS' if not errors else 'FAIL','errors':sorted(set(errors)),'warnings':sorted(set(warnings))}
    if args.json_out:
        args.json_out.parent.mkdir(parents=True,exist_ok=True)
        args.json_out.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(f"public release gate: {result['status']} ({version})")
    for w in result['warnings']: print(f'warning: {w}')
    for e in result['errors']: print(f'error: {e}')
    return 0 if not errors else 1

if __name__=='__main__': raise SystemExit(main())

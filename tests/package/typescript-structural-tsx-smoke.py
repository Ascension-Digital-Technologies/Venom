#!/usr/bin/env python3
import json, pathlib, subprocess, tempfile
root = pathlib.Path(__file__).resolve().parents[2]
driver = root / 'tools' / 'typescript_structural_frontend.js'
with tempfile.TemporaryDirectory() as td:
    td = pathlib.Path(td)
    src = td / 'fixture.tsx.venom-input'
    out = td / 'fixture.jsx'
    meta = td / 'fixture.json'
    src.write_text('''type Props = { title: string };\nconst Card = ({ title }: Props) => <section data-kind="card"><h1>{title}</h1></section>;\nexport { Card };\n''')
    subprocess.run(['node', str(driver), str(src), str(out), str(meta)], check=True)
    js = out.read_text()
    data = json.loads(meta.read_text())
    assert 'type Props' not in js
    assert '<section' in js and '<h1>' in js
    assert data['tsx'] is True
print('structural TSX frontend smoke passed')

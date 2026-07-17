#!/usr/bin/env python3
import json, pathlib, subprocess, tempfile
root = pathlib.Path(__file__).resolve().parents[2]
driver = root / 'tools' / 'typescript_structural_frontend.js'
with tempfile.TemporaryDirectory() as td:
    td = pathlib.Path(td)
    src = td / 'fixture.ts.venom-input'
    out = td / 'fixture.js'
    meta = td / 'fixture.json'
    src.write_text('''interface Row { value: number }\nclass Box<T> { constructor(public value: T) {} }\nconst row = { value: 7 } satisfies Row;\nconst box = new Box<Row>(row);\nbox.value.value;\n''')
    subprocess.run(['node', str(driver), str(src), str(out), str(meta)], check=True)
    js = out.read_text()
    data = json.loads(meta.read_text())
    assert 'interface Row' not in js
    assert 'satisfies Row' not in js
    assert 'new Box(row)' in js
    assert 'constructor(value)' in js
    assert 'this.value = value' in js
    assert data['frontend'] == 'typescript-compiler-api'
    assert data['version']
    assert (pathlib.Path(str(out) + '.map')).read_text()
print('structural TypeScript frontend smoke passed')

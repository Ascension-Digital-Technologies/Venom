from pathlib import Path
import sys
root=Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
ps=(root/'scripts/build-quickjs-wasm.ps1').read_text(encoding='utf-8')
assert '--verify-embedded' in ps and '--require-real' in ps
assert "if ($Artifact -ne '')" in ps
assert "if($Artifact -eq ''){throw" not in ps.replace(' ', '')
for name in ('protected-chess','nova-trade','bot-detection'):
    for ext in ('bat','ps1','sh'):
        p=root/'scripts'/f'{name}.{ext}'
        assert p.is_file(), p
    text=(root/'scripts'/f'{name}.ps1').read_text(encoding='utf-8')
    assert "build-and-serve-example.ps1" in text
helper=(root/'scripts/build-and-serve-example.ps1').read_text(encoding='utf-8')
for marker in ('build-site.ps1','http.server','--directory','$Profile'):
    assert marker in helper, marker
print('example launcher and runtime bootstrap smoke: PASS')

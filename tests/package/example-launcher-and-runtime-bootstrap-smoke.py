from pathlib import Path
import sys
root=Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
ps=(root/'scripts'/'windows'/'build-quickjs-wasm.ps1').read_text(encoding='utf-8')
assert '--verify-embedded' in ps and '--require-real' in ps
assert "if($Artifact)" in ps
assert "if($Artifact -eq ''){throw" not in ps.replace(' ', '')
for name in ('protected-chess','nova-trade','bot-detection'):
    for ext in ('bat','ps1','sh'):
        folder='linux' if ext=='sh' else 'windows'
        p=root/'scripts'/folder/f'{name}.{ext}'
        assert p.is_file(), p
    text=(root/'scripts'/'windows'/f'{name}.ps1').read_text(encoding='utf-8')
    assert "build-and-serve-example.ps1" in text
helper=(root/'scripts'/'windows'/'build-and-serve-example.ps1').read_text(encoding='utf-8')
for marker in ('build-site.ps1','http.server','--directory','$Profile'):
    assert marker in helper, marker
print('example launcher and runtime bootstrap smoke: PASS')

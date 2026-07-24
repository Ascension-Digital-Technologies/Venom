from pathlib import Path
import subprocess, sys, tempfile
root=Path(__file__).resolve().parents[2]
mods=sorted((root/'src/templates/turbojs-engine').glob('*.js'))
assert len(mods)>=5
js=''.join(p.read_text(encoding='utf-8') for p in mods)
checked=(root/'src/generated/runtime/turbojs_engine_module.cpp').read_text(encoding='utf-8')
a=checked.index('R"TJSENGINE(')+len('R"TJSENGINE(')
b=checked.index(')TJSENGINE";',a)
assert checked[a:b]==js, 'checked-in TurboJS engine wrapper drifted from authored modules'
with tempfile.TemporaryDirectory() as td:
    out=Path(td)/'turbojs_engine_module.cpp'
    subprocess.run([sys.executable,str(root/'tools/generators/runtime/generate_turbojs_engine_module.py'),
      '--input-dir',str(root/'src/templates/turbojs-engine'),
      '--prefix',str(root/'tools/generators/runtime/templates/turbojs_engine_module.prefix.cpp'),
      '--suffix',str(root/'tools/generators/runtime/templates/turbojs_engine_module.suffix.cpp'),
      '--output',str(out)],check=True)
    assert out.read_text(encoding='utf-8')==checked
    jsfile=Path(td)/'engine.mjs'; jsfile.write_text(js,encoding='utf-8')
    node=subprocess.run(['node','--check',str(jsfile)],capture_output=True,text=True)
    assert node.returncode==0,node.stderr
print('TurboJS engine module bundle: PASS')

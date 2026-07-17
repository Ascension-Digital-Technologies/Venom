from pathlib import Path
import subprocess, sys, tempfile
root=Path(__file__).resolve().parents[2]
mods=sorted((root/'src/runtime/js/quickjs-engine').glob('*.js'))
assert len(mods)>=5
js=''.join(p.read_text(encoding='utf-8') for p in mods)
checked=(root/'src/generated/runtime/quickjs_engine_module.cpp').read_text(encoding='utf-8')
a=checked.index('R"QJSENGINE(')+len('R"QJSENGINE(')
b=checked.index(')QJSENGINE";',a)
assert checked[a:b]==js, 'checked-in QuickJS engine wrapper drifted from authored modules'
with tempfile.TemporaryDirectory() as td:
    out=Path(td)/'quickjs_engine_module.cpp'
    subprocess.run([sys.executable,str(root/'tools/generate_quickjs_engine_module.py'),
      '--input-dir',str(root/'src/runtime/js/quickjs-engine'),
      '--prefix',str(root/'tools/templates/quickjs_engine_module.prefix.cpp'),
      '--suffix',str(root/'tools/templates/quickjs_engine_module.suffix.cpp'),
      '--output',str(out)],check=True)
    assert out.read_text(encoding='utf-8')==checked
    jsfile=Path(td)/'engine.mjs'; jsfile.write_text(js,encoding='utf-8')
    node=subprocess.run(['node','--check',str(jsfile)],capture_output=True,text=True)
    assert node.returncode==0,node.stderr
print('QuickJS engine module bundle: PASS')

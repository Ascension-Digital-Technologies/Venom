from pathlib import Path
import subprocess, sys, tempfile
root=Path(__file__).resolve().parents[2]
modules=sorted((root/'src/templates/browser').glob('*.js'))
assert len(modules)>=7
expected=''.join(p.read_text(encoding='utf-8') for p in modules)
checked=(root/'src/generated/runtime/javascript/browser_runtime.js').read_text(encoding='utf-8')
assert checked==expected, 'checked-in browser runtime bundle drifted from authored modules'
with tempfile.TemporaryDirectory() as td:
    out=Path(td)/'runtime.js'
    subprocess.run([sys.executable,str(root/'tools/generators/runtime/bundle_js_modules.py'),'--input-dir',str(root/'src/templates/browser'),'--output',str(out)],check=True)
    assert out.read_text(encoding='utf-8')==expected
    node=subprocess.run(['node','--check',str(out)],capture_output=True,text=True)
    assert node.returncode==0,node.stderr
print('browser runtime module bundle: PASS')

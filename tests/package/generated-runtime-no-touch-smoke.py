#!/usr/bin/env python3
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='venom-generator-no-touch-') as td:
    out=Path(td)
    cases=[
      ('bundle_js_modules.py',['--input-dir',str(root/'src/templates/browser'),'--output'],out/'runtime.bundle.js'),
      ('generate_turbojs_engine_module.py',['--input-dir',str(root/'src/templates/turbojs-engine'),'--prefix',str(root/'tools/generators/runtime/templates/turbojs_engine_module.prefix.cpp'),'--suffix',str(root/'tools/generators/runtime/templates/turbojs_engine_module.suffix.cpp'),'--output'],out/'turbojs_engine_module.cpp')]
    generator_root = root / 'tools' / 'generators' / 'runtime'
    for tool,args,output in cases:
        command=['python3',str(generator_root/tool),*args,str(output)]
        subprocess.run(command,check=True)
        before=output.stat().st_mtime_ns
        subprocess.run(command,check=True)
        after=output.stat().st_mtime_ns
        assert before == after, f'{output.name} was rewritten despite identical content'
print('generated runtime no-touch smoke passed')

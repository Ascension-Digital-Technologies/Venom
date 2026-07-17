#!/usr/bin/env python3
import json, pathlib, shutil, subprocess, sys, tempfile

exe = pathlib.Path(sys.argv[1]).resolve()
root = pathlib.Path(tempfile.mkdtemp(prefix='venom-cli-smoke-'))
try:
    project = root / 'sample'
    subprocess.run([str(exe), 'new', str(project)], check=True)
    required = ['venom.toml','index.html','assets/js/app.js','assets/css/app.css','protected/engine.js','README.md']
    for rel in required:
        if not (project / rel).is_file(): raise SystemExit(f'missing scaffold file: {rel}')
    subprocess.run([str(exe), 'config', 'validate', str(project/'venom.toml')], check=True)
    result = subprocess.run([str(exe), 'config', 'print', str(project/'venom.toml'), '--format', 'json'], check=True, capture_output=True, text=True)
    payload=json.loads(result.stdout)
    assert payload['profile']=='prod' and payload['runtime']=='quickjs-wasm' and payload['fail_closed'] is True
    runtime=root/'runtime'
    subprocess.run([str(exe), 'runtime', 'install', '--dir', str(runtime)], check=True)
    subprocess.run([str(exe), 'runtime', 'verify', '--dir', str(runtime)], check=True)
    wasm=(runtime/'quickjs-runtime.wasm').read_bytes()
    assert wasm[:4]==b'\0asm' and len(wasm)>100000
    subprocess.run([str(exe), 'runtime', 'update', '--dir', str(runtime)], check=True)
    print('cli project/runtime smoke: PASS')
finally:
    shutil.rmtree(root, ignore_errors=True)

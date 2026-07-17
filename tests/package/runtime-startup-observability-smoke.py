#!/usr/bin/env python3
from pathlib import Path
import json, subprocess
root=Path(__file__).resolve().parents[2]
contract=json.loads((root/'contracts/runtime-startup.json').read_text(encoding='utf-8'))
assert contract['schema']=='VENOM_RUNTIME_STARTUP_V1'
assert contract['retryPolicy']['mode']=='full-page-reload'
loader=(root/'src/compiler/pipeline/js.cpp').read_text(encoding='utf-8')
for token in ['__venomBootTimeline','venom:boot-phase','__venomClassifyBootError','VENOM_BOOT_MODULE_DEPENDENCY','__venomRetryBoot','timeline:__venomFreezeTimeline()']:
    assert token in loader,token
runtime=(root/'src/runtime/js/browser/60-route-runtime.js').read_text(encoding='utf-8')
for phase in ['package-load','package-policy','runtime-install','route-decode','route-render','script-execution','navigation-install']:
    assert f"startPhase('{phase}')" in runtime,phase
    assert f"finishPhase('{phase}'" in runtime,phase
subprocess.run(['node','--check',str(root/'src/runtime/js/browser/60-route-runtime.js')],check=True)
print('runtime startup observability smoke passed')

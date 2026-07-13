#!/usr/bin/env python3
import json, subprocess, sys, tempfile
from pathlib import Path

root=Path(__file__).resolve().parents[2]
exe=Path(sys.argv[1]) if len(sys.argv)>1 else root/'build/v125/venom'
expected=json.loads((root/'contracts/product-contracts.json').read_text())
out=json.loads(subprocess.check_output([str(exe),'contracts','--format','json'],text=True))
for key in ('package_format_version','package_runtime_abi','route_vm_contract','dom_command_contract','quickjs_runtime_abi','host_bridge_abi','configuration_schema','lockfile_schema'):
    if out.get(key)!=expected.get(key): raise SystemExit(f'contract mismatch: {key}: {out.get(key)!r} != {expected.get(key)!r}')
source=(root/'src/compiler/build.cpp').read_text()
for section in ('QuickJsParityProbe','QuickJsParityContract','QuickJsModuleCache','QuickJsResolverAudit','QuickJsExecutionPipeline','QuickJsEvalResults','QuickJsConsoleCapture','QuickJsFailureReports'):
    needle=f'if (!hardened_release_asset) {{\n    add_package_section(venom::package::SectionType::{section}'
    if needle not in source: raise SystemExit(f'{section} is not development-only')
if 'production_metadata_profile=' not in source or 'minimal-v1' not in source: raise SystemExit('minimal metadata profile missing')
print('[venom] product contracts and minimal production metadata: PASS')

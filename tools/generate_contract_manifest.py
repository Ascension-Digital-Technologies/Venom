#!/usr/bin/env python3
"""Generate the stable Venom product-contract manifest."""
from __future__ import annotations
import argparse, json, re
from pathlib import Path

def parse_header(path: Path) -> dict:
    text=path.read_text(encoding='utf-8')
    def num(name):
        m=re.search(rf'{name}\s*=\s*(\d+)u?', text)
        if not m: raise SystemExit(f'missing contract constant: {name}')
        return int(m.group(1))
    def string(name):
        m=re.search(rf'{name}\s*=\s*"([^"]+)"', text)
        if not m: raise SystemExit(f'missing contract constant: {name}')
        return m.group(1)
    return {
      'schema_version': 1,
      'stability': 'stable-v1',
      'contracts': {
        'package_format': num('kPackageFormatVersion'),
        'package_runtime_abi': num('kPackageRuntimeAbi'),
        'route_vm': string('kRouteVmContract'),
        'dom_commands': string('kDomCommandContract'),
        'quickjs_module_bundle': string('kQuickJsModuleBundleContract'),
        'quickjs_runtime_abi': num('kQuickJsRuntimeAbi'),
        'host_bridge_abi': num('kHostBridgeAbi'),
        'configuration_schema': num('kConfigurationSchema'),
        'lockfile_schema': num('kLockfileSchema'),
        'public_bridge_api': 1,
        'annotation_grammar': 1,
        'production_asset_layout': 1,
      },
      'compatibility_policy': {
        'numeric_contracts': 'must not decrease within a major release',
        'named_contracts': 'must not change within a major release',
        'removed_contracts': 'forbidden within a major release'
      }
    }

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--header', type=Path, required=True)
    ap.add_argument('--version', required=True)
    ap.add_argument('--out', type=Path, required=True)
    a=ap.parse_args()
    data=parse_header(a.header); data['product_version']=a.version
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(data, indent=2, sort_keys=True)+'\n', encoding='utf-8')
if __name__=='__main__': main()

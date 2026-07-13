#!/usr/bin/env python3
from pathlib import Path
import re
root=Path(__file__).resolve().parents[2]

def project_version(text):
    match = re.search(r'project\(venom\s+VERSION\s+(\d+)\.(\d+)\.(\d+)', text, re.S)
    return tuple(map(int, match.groups())) if match else (0, 0, 0)
cmake=(root/'CMakeLists.txt').read_text()
package=(root/'tools/package_release.py').read_text()
verify=(root/'tools/verify_release.py').read_text()
meta=(root/'tools/generate_release_metadata.py').read_text()
checks={
 'v1.0.19+': project_version(cmake) >= (1, 0, 19),
 'source date epoch':'SOURCE_DATE_EPOCH' in package and 'source_date_epoch' in package,
 'deterministic archives':'ZipInfo' in package and 'gzip.GzipFile' in package,
 'sbom generation':'SBOM.cdx.json' in meta and 'CycloneDX' in meta,
 'provenance generation':'PROVENANCE.intoto.json' in meta and 'slsa.dev/provenance' in meta,
 'strict metadata verification':'--require-supply-chain-metadata' in verify,
 'toolchain lock':(root/'toolchains.lock.json').is_file(),
 'release CI':(root/'.github/workflows/release-hardening.yml').is_file(),
}
failed=[k for k,v in checks.items() if not v]
if failed: raise SystemExit('phase6 supply-chain smoke failed: '+', '.join(failed))
print('phase6 release supply-chain hardening smoke passed')

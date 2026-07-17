from __future__ import annotations

from pathlib import Path

REQUIRED_PROVENANCE = (
    'module_bundle_contract=VQJSMB04',
    'literal_dynamic_import=true',
)


def artifact_is_current(root: Path) -> bool:
    header = (root / 'src' / 'generated' / 'runtime' / 'quickjs_runtime_wasm_blob.hpp').read_text(encoding='utf-8')
    return all(token in header for token in REQUIRED_PROVENANCE)


def require_current_artifact(root: Path) -> None:
    if artifact_is_current(root):
        return
    print('SKIP: embedded QuickJS/WASM lacks VQJSMB04 literal dynamic-import provenance; rebuild with scripts/build-quickjs-wasm.*')
    raise SystemExit(77)

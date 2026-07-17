# Phase 1A — Protected-Intent Closure Enforcement

Date: 2026-07-14

## Implemented

Production builds now fail with `VNM-PROT-1001` whenever a function-level protected bridge candidate is not successfully extracted into the protected QuickJS registry. This closes the verified path where an explicitly protected arrow function could remain in browser JavaScript while the production build succeeded.

The compiler now preserves the authoritative function-runtime records captured before later chunk rewrites and uses those records for extraction analysis. Development builds retain diagnostics; production builds throw before package emission.

`docs/audit` is now accepted by `tools/documentation_gate.py`.

## Compatibility

Named protected function declarations continue to use the existing extraction path. Explicitly isolated named declarations remain supported for large self-contained engines, including the protected chess engine. Unsupported arrow-function syntax fails before the isolated dependency override can apply.

## Changed files

- `src/compiler/pipeline/js.cpp`
- `src/compiler/pipeline/js_rewriting.cpp`
- `tools/documentation_gate.py`
- `tests/fixtures/sites/protected-intent-arrow/*`
- `tests/package/protected-intent-closure-smoke.py`

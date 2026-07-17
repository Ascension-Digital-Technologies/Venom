# Phase 2B: Exported and Multiline Arrow Lowering

Date: 2026-07-14

## Summary

Venom now recognizes and lowers protected variable-bound arrows whose declarations are exported or whose parameter headers span multiple lines. Exported browser APIs are preserved as exported asynchronous bridge functions while their original implementations are compiled into the protected QuickJS registry.

## Implemented

- `export const`, `export let`, and `export var` protected arrows.
- Multiline parenthesized arrow parameter lists.
- Export-aware dependency analysis that removes the `export` modifier before lexical dependency resolution.
- Export-preserving browser bridge generation.
- Permanent regression fixture and smoke test.

## Security invariants

- Unsupported syntax remains fail-closed.
- Unawaited cross-realm calls remain rejected.
- Each successful protected declaration resolves to exactly one intent and one registry envelope.
- Original protected implementations are removed from browser-executable JavaScript.

## Verification

- Native Release compiler build: PASS.
- Existing protected arrow closure smoke test: PASS.
- Unawaited protected call smoke test: PASS.
- Exported multiline protected arrow smoke test: PASS.
- Protected chess development build: PASS.
- Bot-detection development build: PASS.

## Residual boundary

Class methods and object-literal methods are not lowered yet. Correct support requires receiver-state semantics for `this`, private fields, `super`, prototypes, and method identity. Treating those methods as standalone functions would change behavior and weaken the protection guarantee, so they remain unsupported and fail closed.

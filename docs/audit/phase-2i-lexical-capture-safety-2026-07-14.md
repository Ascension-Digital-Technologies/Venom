# Phase 2I — Lexical Capture Safety

## Scope

This pass hardens protected-function dependency analysis and narrows the `isolated` escape hatch.

## Implemented

- Classifies top-level primitive constants, mutable bindings, unsupported constant initializers, imports, and helper functions.
- Rejects mutable captures, browser-only captures, unsupported constants, and imports that have not been lowered through the protected module graph.
- Preserves automatic lifting of primitive immutable constants and pure helper functions.
- Adds nested-function and arrow-parameter recognition.
- Masks regular-expression literals during identifier scanning.
- Includes the first unresolved function and reason in `VNM-PROT-1004`.
- Allows the large chess engine's explicit `isolated` unit only after separate runtime-safety verification; small units cannot use this fallback.

## Verification

- Native Release compiler build: PASS
- Lexical capture safety smoke: PASS
- Function dependency lifting smoke: PASS
- Protected chess development build: PASS
- Bot detection development build: PASS
- Documentation gate: PASS

## Residual risk

This remains a syntax-aware scanner rather than a standards-complete ESTree scope graph. Complex block shadowing, imported binding aliasing, and class receiver state should remain fail closed until a full parser-backed scope model is integrated.

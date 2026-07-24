# Phase 2A — Syntax-aware protected arrow lowering

Date: 2026-07-14

## Implemented

The mixed-runtime rewrite pipeline now recognizes and extracts:

- named function declarations;
- `const`, `let`, and `var` arrow functions;
- `async` arrow functions;
- block-bodied and expression-bodied arrows;
- single-parameter and parenthesized parameter forms.

Extraction uses token-aware balanced range scanning for function and arrow bodies instead of the previous declaration-only line regex. The protected implementation is moved into the TurboJS bridge registry and the browser receives an asynchronous opaque-ID bridge stub.

## Security invariants retained

- protected calls outside the declaration must be explicitly awaited;
- unresolved dependencies remain fail-closed;
- unsupported syntax remains fail-closed in production;
- one resolved arrow intent maps to the protected registry and one TurboJS envelope;
- original protected declaration text is checked for removal from the browser chunk.

## Verification

- native Release compiler build: PASS;
- protected async arrow fixture: PASS;
- closure requested=1, resolved=1, expected records=1: PASS;
- unawaited arrow call rejected with VNM-PROT-1001: PASS;
- protected-chess development build: PASS;
- bot-detection development build: PASS.

## Residual work

This is the first syntax-aware lowering slice, not yet a complete standards-compliant JavaScript AST. Class methods, object methods, destructuring-heavy parameter syntax, multiline arrow headers, generators, and all export variants still require the planned parser expansion. Scope and dependency analysis continues to use the existing lexical dependency resolver.

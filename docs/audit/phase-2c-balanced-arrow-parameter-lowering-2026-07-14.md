# Phase 2C — Balanced Arrow Parameter Lowering

Date: 2026-07-14

## Summary

Protected variable-bound arrow extraction no longer relies on a `[^)]*` regular-expression parameter match. The compiler now locates parenthesized parameter lists with balanced JavaScript token scanning before requiring the `=>` token.

## Supported additions

- nested object destructuring
- nested array destructuring
- default parameter expressions containing parentheses
- object and array literals in defaults
- multiline exported async arrows using these forms

## Security behavior

The existing protection-intent closure remains fail closed. A declaration that cannot be parsed or linked to a protected registry record is not silently retained in production browser JavaScript.

## Verification

- native Release compiler build: PASS
- nested/default protected-arrow fixture: PASS
- exported multiline protected-arrow fixture: PASS
- function bridge extraction smoke test: PASS
- protected-chess development compatibility build: PASS
- bot-detection development compatibility build: PASS

## Residual limitations

Class methods, object methods, private fields, `super`, and receiver-bound `this` semantics remain unsupported for function-level extraction. These forms must continue to fail closed until receiver identity and state transfer are explicitly designed.

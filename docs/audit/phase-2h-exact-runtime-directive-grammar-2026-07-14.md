# Phase 2H: Exact Runtime Directive Grammar

Date: 2026-07-14

## Summary

Venom previously selected runtime directives using substring searches. That allowed malformed values such as `@venom: protectedd` or `@venom: notbrowser` to be interpreted as valid runtime requests.

This pass replaces substring selection with an exact grammar for file- and function-level directives.

## Accepted forms

- `@venom: browser`
- `@venom: protected`
- `@venom: protected isolated` for function declarations
- `@venom: protected module` at file scope

## Rejected forms

- Runtime-like misspellings or suffixes
- Unknown modifiers
- `browser isolated`
- `browser module`
- `protected isolated module`
- Function-level `protected module`

## Security effect

A typo can no longer silently select an unintended runtime. Runtime-like malformed directives fail before planning, rewriting, QuickJS compilation, or package emission.

## Verification

- Clean native Release build: PASS
- Exact grammar smoke test: PASS
- Runtime conflict regression: PASS
- Deterministic binding regression: PASS
- Protected module graph regression: PASS

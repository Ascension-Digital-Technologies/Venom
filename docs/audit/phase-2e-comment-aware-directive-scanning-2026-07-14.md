# Phase 2E — Comment-aware directive scanning

## Problem

Function-level planning searched complete source lines for `@venom:`. A string or template literal containing documentation such as `"// @venom: protected"` could therefore be interpreted as a compiler directive and bind to a later declaration.

## Fix

The planner now lexically tracks single-quoted strings, double-quoted strings, template literals, escapes, line comments, and block comments. Only text extracted from a JavaScript comment is eligible to carry a function-level Venom directive.

Both single-line and multiline/JSDoc block comments are supported. Directive-like text in executable literals is inert.

## Validation

- Native Release compiler build passed.
- Comment-only directive fixture produced exactly one requested and resolved protected intent.
- False directives in strings and templates produced no intents.
- Function bridge extraction, exported multiline arrows, and nested/default arrows passed.
- Protected Chess and Bot Detection development builds passed.

## Residual risk

This is a focused lexical scanner, not a complete ECMAScript parser. Template interpolation and future syntax forms should eventually be handled by the planned full AST front end. Unsupported or ambiguous protected lowering remains fail closed.

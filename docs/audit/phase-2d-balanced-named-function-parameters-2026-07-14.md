# Phase 2D: Balanced named-function parameters

Venom now parses named protected-function parameter lists with balanced JavaScript token ranges rather than a `[^)]*` regular expression. This supports nested defaults, object/array destructuring, and multiline parameters while preserving fail-closed lowering.

Generator and async-generator declarations remain unsupported because the current Promise-based bridge cannot preserve iterator identity, incremental yield state, `return()`, or `throw()` semantics.

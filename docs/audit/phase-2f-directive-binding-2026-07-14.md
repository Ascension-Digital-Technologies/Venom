# Phase 2F — Deterministic directive binding

Function-level Venom directives now bind only to the immediately following supported declaration, while allowing whitespace and intervening comments. Consecutive runtime directives and directives followed by unrelated executable syntax fail closed instead of being silently discarded or attached to a later function.

## Security properties

- Directive text remains comment-only.
- Whitespace and documentation comments may appear before the declaration.
- An intervening executable statement makes the directive orphaned.
- Consecutive runtime directives are rejected as ambiguous.
- A directive at end of file is rejected.

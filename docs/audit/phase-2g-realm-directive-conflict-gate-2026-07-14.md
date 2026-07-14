# Phase 2G — Realm Directive Conflict Gate

Venom now rejects ambiguous realm selection before JavaScript planning. A source file may not declare both `@venom: browser` and `@venom: protected` at file scope, and an HTML `data-venom` attribute may not contradict the file-scope directive.

The gate runs during script discovery, before browser evidence, function extraction, bytecode generation, or package emission. This prevents precedence order from silently selecting one realm.

## Diagnostics

- `conflicting file-scope Venom directives`
- `conflicting HTML data-venom and file-scope Venom directives`

## Regression

`tests/package/realm-directive-conflict-smoke.py` verifies both conflict forms fail closed.

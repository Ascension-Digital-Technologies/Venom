# Phase 3A — Least-Privilege Capability Manifests

Date: 2026-07-14
Version: 2.0.0-alpha.20

## Summary

The host capability metadata is now specialized per JavaScript chunk instead of granting every chunk the complete browser capability set. The default is reduced to `__venomRuntime`, and additional capabilities are declared only when the chunk references their corresponding browser APIs.

## Updated protected chess example

The user-supplied protected chess example replaces the repository example. Its directives were corrected from two adjacent file-scope directives to an explicit browser file realm followed by a function-level protected directive:

```js
// @venom: browser
"use strict";

// @venom: protected isolated
function runChessEngine(request) {
```

This is valid under the exact directive grammar and keeps only `runChessEngine` in the protected QuickJS registry.

## Capability policy

- Default capability: `__venomRuntime`
- Undeclared capability policy: deny
- Per-chunk inferred capabilities: console, document, window, navigator, fetch, timers, events
- Protected chess engine capability set: `__venomRuntime`

## Verification

- Native Release compiler build: PASS
- Updated protected chess development build: PASS
- QuickJS bytecode records: 1
- Protected engine capability manifest: `__venomRuntime`
- Browser libraries receive specialized capability lists

## Residual risk

This phase specializes and binds capability manifests. The next phase must enforce those manifests at the runtime host-call dispatch boundary. Metadata alone is not a security boundary.

# Phase 1C — Unified Protection Closure

Date: 2026-07-14

## Implemented

- Whole-file protected JavaScript chunks now produce build-only protection-intent ledger entries.
- The closure summary records requested, resolved, whole-file, and expected QuickJS record counts.
- `verify` independently reads `build.json` and compares requested versus resolved intents.
- `verify` compares expected protected QuickJS records against records found across all decoded package sections, including lazy script sections.
- Detailed source paths and symbols remain outside `dist` under the sibling `.venom/<output>/protection-intents.json` report.

## New release failures

- Missing protection-closure metadata.
- Requested and resolved intent count mismatch.
- Emitted QuickJS bytecode record count mismatch.

## Verification

- Clean Release compiler build: PASS.
- Unsupported protected arrow function production rejection: PASS.
- Protected chess development closure: requested 1, resolved 1, expected records 1: PASS.
- Whole-file protected fixture: requested 1, resolved 1, whole-file intents 1, expected records 1: PASS.
- Whole-file package inspection finds one QuickJS envelope across lazy sections: PASS.
- Documentation gate: PASS.

## Residual risk

The closure summary is integrity-bound through the existing package/build asset chain but is not yet signed by an external publisher key. Whole-file intent IDs are deterministic build-local opaque identifiers rather than cryptographic identities. AST-based extraction and external release signing remain later work.

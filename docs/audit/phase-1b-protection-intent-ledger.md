# Phase 1B — Protection Intent Ledger

Date: 2026-07-14

## Implemented

The compiler now creates one build-only JSON ledger entry for every protected bridge rewrite record. Each entry links the protected request to its opaque candidate ID, terminal lowering status, and protected registry record ID.

Production closure enforcement now rejects empty candidate IDs, duplicate candidate IDs, unresolved records, and resolved intents without a native QuickJS registry record. New diagnostics are `VNM-PROT-1002` through `VNM-PROT-1005`.

The public build metadata contains only aggregate closure data (`requested`, `resolved`, and `registry_present`). Source paths and symbols remain in the private sibling report directory `.venom/<output-name>/protection-intents.json`, outside the distribution directory.

## Verification

- Clean Release build: PASS.
- Unsupported protected arrow function production rejection: PASS (`VNM-PROT-1001`).
- Protected chess development build: PASS.
- Chess ledger: one requested intent, one resolved intent, one registry record, symbol `runChessEngine`.
- Public build metadata contains aggregate closure counts only.

## Environment limitation

Historical note: at the time of this audit, the former external JavaScript hardener dependency prevented a fresh protected-chess production rebuild in the audit container. The hardener is now embedded natively and this limitation no longer applies. The compiler reached and passed the new closure checks in the development chess build. The prior Phase 1A production chess regression remains unchanged by the ledger patch, but should be rerun on the normal release machine before publishing.

## Residual risk

The ledger currently covers function/module bridge rewrite records. Whole-file protected chunks are represented by package bytecode records but do not yet receive individual intent entries. A later pass should unify whole-file, extracted-function, and remote-operation intents under one schema and make `release-check` independently compare the signed package closure summary against the private compiler ledger.

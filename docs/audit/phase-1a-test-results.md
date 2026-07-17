# Phase 1A Test Results

Date: 2026-07-14

- PASS: clean Release build on Linux/GCC.
- PASS: protected browser-file arrow fixture is rejected in `prod`.
- PASS: failure contains `VNM-PROT-1001`.
- PASS: documentation publication gate accepts `docs/audit`.
- PASS: protected-chess production build completes with `--hashed --strict-release`.
- PASS: protected-chess `release-check --target browser` reports `release_status: PASS`.

The existing dependency-lifting smoke test expects debug reports at an outdated output path and did not complete; this is a pre-existing test-path mismatch rather than a compiler failure observed in this pass.

# Phase 1A Fixed Findings

## VNM-P1-001 — Explicit protected function may remain browser-side

Status: FIXED for unresolved function-level bridge candidates in production.

A production build now terminates before package emission whenever a protected candidate has a rewrite status other than `extracted`.

Regression fixture: `tests/fixtures/sites/protected-intent-arrow`.

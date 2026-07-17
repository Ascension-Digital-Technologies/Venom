# Phase 1E — QuickJS Record Count Fix

## Finding

`verify` counted every decoded occurrence of `VQJSE006`. The runtime metadata section documents `package_envelope_magic=VQJSE006`, so a distribution with one executable protected record was reported as containing two records. The protection-closure gate then rejected the valid build.

## Fix

QuickJS envelope markers are now counted only in executable/package payload sections. Integrity and manifest sections are excluded from executable record accounting. Metadata remains available for runtime validation without being interpreted as bytecode.

## Verification

- Linux Release compiler build: PASS
- Whole-file protected fixture: expected records 1, detected records 1
- Metadata occurrence no longer increments the executable count
- Closure mismatch failure removed

## Windows regression expected

Re-run `scripts\examples\protected-chess-prod.ps1`. The production gate should report `quickjs_bytecode_records: 1`, `protection_expected_quickjs_records: 1`, and `release_status: PASS`.

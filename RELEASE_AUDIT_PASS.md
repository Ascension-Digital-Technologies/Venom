# Venom 3.0.0 release audit pass

## Corrected

- Production package sections now consume only reachability-approved `AssetPipeline` records.
- Unrelated non-code project files are no longer inserted into production VBC packages.
- Sensitive production asset paths and extensions are rejected fail-closed.
- Release installation smoke fixture now models the required hardener-worker binary.
- Current v3 release tests were aligned with the hash-only asset layout and current generated file paths.
- Compatibility matrix smoke now validates schema version 3.
- Capability graph smoke now uses the source compatibility-analysis path rather than the distribution analyzer.

## Verified

- Complete native Release build: PASS.
- Planned public assets only smoke: PASS.
- Release installation smoke: PASS.
- Production-only build smoke: PASS.
- TurboJS WASM stack safety smoke: PASS.
- TurboJS module artifact gate: PASS.
- Release package metadata redaction smoke: PASS.
- Compatibility matrix smoke: PASS.
- Capability graph smoke: PASS.

## Remaining audit work

The broader historical test suite still contains additional obsolete fixtures and tests that expect private `build.json` reports in public production output. Those should be migrated to explicit private report directories rather than restoring debug metadata to release distributions.

# Prioritized Security Remediation Plan

## Priority 0 — Correct security claims and entropy sources

Target: before any production-grade claim.

1. Replace native `keygen` randomness with an OS CSPRNG or libsodium.
2. Replace bot session token generation with secure host-provided entropy.
3. Rename browser `VAEAD001` and all related “AEAD/encryption/authentication” claims to reversible browser protection terminology.
4. Add automated tests that reject audited-crypto claims in browser builds.

Acceptance criteria:

- Key generation fails if secure entropy is unavailable.
- Two million generated token samples show no collisions in test environments, with provider metadata recorded.
- Browser reports never claim secret-key cryptography.
- Documentation clearly distinguishes browser and native threat models.

## Priority 1 — Remove downgrade and key-loading risks

1. Remove `VSEAL001` from production native and WASM runtimes.
2. Statically link libsodium or load it only from a configured trusted path.
3. Replace environment-key guidance with secure key-file/secret-provider guidance.
4. Explicitly wipe process-global key buffers.

Acceptance criteria:

- Production runtime unit tests reject every legacy envelope.
- Library-path hijack tests fail to load an untrusted local DLL/shared object.
- Key material is absent from logs, arguments, child environments, and post-clear memory tests where observable.

## Priority 2 — Strengthen browser deployment defenses

1. Emit a CSP deployment template for each build.
2. Run all browser compatibility fixtures under the generated CSP.
3. Add signed public metadata for offline publisher verification.
4. Add AST-level protected-source leakage checks.

Acceptance criteria:

- Protected examples run without `unsafe-eval`.
- CSP test failures block release.
- Protected function bodies have no normalized AST match in browser chunks.
- Signed release metadata verifies with a pinned public key outside the mutable dist tree.

## Priority 3 — Native parser and binary hardening

1. Enable stack protection, PIE/ASLR, RELRO, fortified libc, CFG/CET where available.
2. Add release binary-hardening inspection.
3. Expand libFuzzer corpora for package, section envelope, bridge frame, and bytecode record parsers.
4. Run sanitizer tests on Linux, Windows clang-cl, and macOS.

Acceptance criteria:

- Release artifacts pass platform-specific mitigation checks.
- Fuzz jobs run continuously with stored regression corpora.
- No sanitizer findings across the mandatory suite.

## Priority 4 — Supply-chain and assurance maturity

1. Generate an SPDX or CycloneDX SBOM.
2. Add vendored dependency provenance and automated review.
3. Pin GitHub Actions by immutable commit SHA for high-assurance release workflows.
4. Commission an independent penetration test and cryptographic design review.

Acceptance criteria:

- Every release includes a signed SBOM.
- Every third-party source and binary has version, origin, license, and digest metadata.
- Independent findings are tracked to closure in this directory.

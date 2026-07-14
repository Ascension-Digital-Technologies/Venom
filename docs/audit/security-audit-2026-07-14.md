# Venom 2.0.0-alpha.2 Security Audit

**Audit date:** 2026-07-14  
**Scope revision:** telemetry-hardened source derived from `venom-secure-web-runtime-v2.0.0-alpha.2`  
**Assessment type:** source-assisted architecture, implementation, build, and test review  
**Overall risk:** **High until the browser cryptography language and native key generation findings are corrected**

## Executive summary

Venom has a strong set of defensive engineering controls for an alpha project: protected JavaScript is compiled to QuickJS bytecode, production builds deny host-JavaScript fallback, runtime assets are hash-bound, source maps and known leak markers are rejected, protected bridge calls are constrained, mutation tests exist, CodeQL and sanitizer workflows are configured, and release publication supports signatures and reproducibility checks.

The audit nevertheless found security gaps that should be addressed before describing the software as production-grade. The most important issue is that browser package sections use a custom reversible construction named `VAEAD001`. Its keys are fixed constants embedded in the compiler and runtime, its nonces are derived from plaintext, and its tags use FNV-based mixing. It can increase reverse-engineering effort, but it is not cryptographic confidentiality or attacker-resistant authenticity. The current threat-model metadata partly acknowledges this limitation, but names such as “AEAD,” “encrypted,” and “authentication” can still create an unsafe expectation.

The native private-key mode is materially stronger because it uses libsodium XChaCha20-Poly1305 with an external key. However, the bundled `keygen` path uses `std::random_device`, which is not guaranteed by C++ to be a cryptographically secure generator. The same issue affects some build-diversification and binding-token generation. The protected bot-detection session also derives its nonce from `Date.now()` and `Math.random()`, so the replay controls are sequencing safeguards rather than cryptographic challenge binding.

No direct remote-code-execution defect was confirmed during this review. The audit did not run a full independent penetration test, browser exploit campaign, or exhaustive native fuzzing session.

## Scope

Reviewed areas:

- CLI parsing and build profiles
- JavaScript annotation and extraction pipeline
- QuickJS bytecode creation and protected export bridge
- Browser loader, worker, and runtime templates
- Native package reader/runtime
- Package writer, section sealing, hashing, compression, and layout polymorphism
- Release inspection and production leak gates
- Bot-detection and protected-chess examples
- CMake policy, compiler warnings, sanitizers, fuzz targets, and CI workflows
- Release signing, reproducibility, and dependency handling
- Security documentation and stated threat model

Out of scope:

- Security of the user’s web server, TLS termination, hosting account, or origin
- Undisclosed browser and QuickJS zero-days
- Formal cryptographic proof
- Exhaustive third-party dependency vulnerability enumeration
- Server-backed key service, because the audited configuration is client-executable
- Physical attacks and hostile operating-system administrators

## Threat model conclusions

### Venom can reasonably protect against

- Casual “view source” copying
- Straightforward asset scraping
- Accidental source-map or original-source publication
- Basic static signature matching across polymorphic builds
- Silent fallback from protected execution into browser `eval` or `Function`
- Simple package corruption and many bridge-frame mutations
- Unbounded bridge payloads and pending request growth

### Venom cannot guarantee protection against

- A determined attacker controlling the browser and DevTools
- Runtime instrumentation of WebAssembly memory and host calls
- Modified browser engines, extensions with sufficient privileges, or OS-level tracing
- A compromised origin that can replace `index.html`, loader, runtime, package, and WASM together
- Exact secrecy of client-delivered logic or constants
- Cryptographic authenticity using only secrets embedded in client-side code

The product should consistently describe browser protection as **reverse-engineering resistance**, not secrecy or trusted execution.

## Positive controls observed

1. Production QuickJS records are required for protected functions, and the build fails when protected exports produce zero records.
2. Host-JavaScript fallback is denied in protected release profiles.
3. The release inspector checks for protected source leakage, source maps, canonical internal names, missing assets, ABI mismatches, and stale runtime artifacts.
4. Browser bridge payloads have size, depth, node-count, pending-call, timeout, and prototype-pollution limits.
5. QuickJS/WASM runtime provenance and required exports are verified.
6. Browser assets use SHA-256 binding and generated HTML supports SRI attributes.
7. Native private-key packages use libsodium XChaCha20-Poly1305 with associated data.
8. CI includes CodeQL, ASan/UBSan, compatibility tests across Chromium/Firefox/WebKit, reproducibility checks, and signed release-set verification.
9. Mutation, malformed package, fuzz-corpus, source-leak, and runtime-assurance tests are extensive.
10. The security documentation explicitly acknowledges that browsers cannot keep a package key secret.

## Findings summary

| ID | Severity | Finding | Status |
|---|---|---|---|
| VSA-001 | High | Browser `VAEAD001` is custom reversible obfuscation, not secure AEAD | Open |
| VSA-002 | High | Native key generation relies on `std::random_device` | Open |
| VSA-003 | High | Bot challenge/session nonces use `Math.random()` and timestamps | Open |
| VSA-004 | Medium | Production runtimes retain legacy `VSEAL001` decoding paths | Open |
| VSA-005 | Medium | Dynamic libsodium loading can permit library search-order abuse | Open |
| VSA-006 | Medium | Package keys can persist in process environment and global strings | Open |
| VSA-007 | Medium | Client integrity has no immutable browser-side trust anchor | Accepted limitation / hardening needed |
| VSA-008 | Medium | CSP is deployment guidance rather than an emitted/enforced profile | Open |
| VSA-009 | Medium | Native release hardening flags are not explicitly standardized | Open |
| VSA-010 | Low | Vendored example libraries lack an automated SBOM/vulnerability gate | Open |
| VSA-011 | Low | Security-sensitive terminology can overstate browser guarantees | Open |
| VSA-012 | Low | Some diversification randomness uses implementation-defined entropy | Open |

Detailed evidence and remediation guidance are maintained in [findings-register.md](findings-register.md).

## Architecture assessment

### Compiler and extraction boundary

The protected-function extraction design is substantially improved by requiring a real bytecode record and scanning browser bundles for implementation markers. The strongest remaining architectural risk is completeness: marker-based leak detection is necessarily heuristic. A protected source can leak without containing a selected marker, or a generated browser chunk can preserve semantically equivalent code with changed identifiers.

Recommended improvement: retain a build-only abstract-syntax fingerprint for every protected function and scan browser chunks using normalized AST hashes, literal-set similarity, and dependency closure verification. Production output must not contain that build-only map.

### QuickJS/WASM execution

Fail-closed QuickJS/WASM execution is a strong control. The runtime still executes on an attacker-controlled client, so bytecode, memory, and bridge behavior remain observable. WASM memory should be assumed readable by a sufficiently motivated attacker.

Recommended improvement: add instruction, wall-clock, heap, recursion, and output quotas at the QuickJS context level—not only at the browser request layer—and test forced termination during loops, allocation storms, deep recursion, and promise queues.

### Browser bridge

The bridge validates shape and size on both sides and rejects dangerous prototype keys. This is good defense in depth. Numeric capability/slot identifiers and opaque per-build export IDs reduce semantic leakage but are not authorization boundaries against code already executing in the same origin.

Recommended improvement: use unforgeable per-session capability material generated with `crypto.getRandomValues`, bind every frame to the exact runtime/package digest, and rotate bridge state after worker restart.

### Package format and integrity

The package has extensive structure validation, section hashes, asset binding, SHA-256 metadata, polymorphic ordering, and tamper checks. FNV-1a is still used for internal package and section hashes in several places. FNV is suitable for accidental corruption detection and indexing, not adversarial integrity.

Recommended improvement: make SHA-256 or BLAKE2/BLAKE3 the authoritative integrity digest everywhere. Keep FNV only for non-security indexing, clearly named as such.

### Native private-key mode

The libsodium mode is the strongest confidentiality option in the codebase. Associated data binds section type, size, name digest, and name. The design correctly requires an external key and disallows this mode for ordinary browser delivery.

Recommended improvement: link libsodium statically or load it from an explicitly trusted absolute path, use an OS CSPRNG for key generation, avoid environment variables for long-lived secrets, and lock/wipe key memory where practical.

### Build and release pipeline

The repository has unusually broad release and compatibility evidence for an alpha project. The stale-compiler correction, mandatory release checks, signed release sets, and reproducibility workflow are valuable.

Recommended improvement: add dependency review, SBOM generation, compiler hardening verification, secret scanning, and a release job that proves all native binaries have the expected hardening properties.

## Test and verification gaps

The following tests should be added or elevated to mandatory release gates:

- Browser package decoder known-answer tests proving `VAEAD001` is classified only as obfuscation
- OS-CSPRNG failure tests and keygen entropy-source assertions
- Bot session nonce uniqueness/predictability tests
- Production rejection of `VSEAL001` inside both native and WASM runtimes
- DLL/shared-library search-order tests
- CSP compatibility tests with no `unsafe-eval` and minimal `unsafe-inline`
- QuickJS termination tests for infinite loops, microtask starvation, and allocation limits
- AST-level protected-source leakage detection
- Cross-build differential analysis measuring stable strings and constants
- Full SBOM and vendored asset inventory verification
- Native binary hardening checks: ASLR, DEP/NX, CFG where available, stack protection, RELRO, PIE, and fortified libc calls

## Severity rationale

- **Critical:** practical compromise of users or build/signing infrastructure with little precondition.
- **High:** defeats a core security claim or creates cryptographic/key-management risk.
- **Medium:** meaningful defense-in-depth weakness requiring a capable attacker or additional precondition.
- **Low:** hardening, assurance, documentation, or maintainability weakness with limited direct exploitability.

## Release recommendation

Do not label this alpha as cryptographically secure client-side code protection. A limited preview can be published after clearly documenting the browser threat model. Before a production-grade release, resolve VSA-001 through VSA-006, add enforced CSP and native binary-hardening profiles, and commission an independent review of the package parser, bridge protocol, QuickJS host API, and native key mode.

## Audit limitations

This report is based on source review and repository-provided assurance mechanisms. Passing tests and static analysis reduce risk but do not prove absence of vulnerabilities. The report must be refreshed after material changes to package crypto, runtime parsing, QuickJS integration, host capabilities, release signing, or protected-function extraction.

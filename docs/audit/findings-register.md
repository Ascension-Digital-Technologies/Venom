# Security Findings Register

## VSA-001 — Browser `VAEAD001` is not cryptographic AEAD

**Severity:** High  
**Affected components:** `src/package/crypto.cpp`, `src/runtime/package_runtime.c`, `src/runtime/wasm_runtime.c`, browser package metadata

### Evidence

The browser section sealer uses fixed constants compiled into both producer and consumer. It derives two “nonces” deterministically from section metadata and plaintext, generates a stream with custom 64-bit mixing, and computes tags with FNV-1a-based constructions. No secret unavailable to the browser is involved.

### Impact

Anyone with the distributed runtime can reproduce the decoding algorithm. The construction does not provide cryptographic confidentiality against a client attacker, and its custom tag is not a substitute for a standard MAC. Calling it AEAD or authenticated encryption risks consumers treating obfuscation as cryptographic protection.

### Recommendation

- Rename `VAEAD001` and related metadata to explicitly identify a reversible browser encoding/obfuscation envelope.
- Never claim confidentiality, authenticity, or audited cryptography for browser packages.
- Use a standard public-key signature for publisher authenticity where verification has a separately trusted public key.
- Reserve XChaCha20-Poly1305 for native/private-key or server-assisted modes.
- Add documentation and tests that prevent browser builds from setting `audited_crypto=true`.

## VSA-002 — Native package key generation uses `std::random_device`

**Severity:** High  
**Affected component:** `src/compiler/pipeline/security.cpp`

### Evidence

`generate_key_file` fills each key byte with the low eight bits of `std::random_device`. The C++ standard does not guarantee that `random_device` is backed by a cryptographically secure operating-system generator on every supported toolchain.

### Impact

A weak or deterministic implementation could produce predictable 256-bit package keys, defeating the confidentiality of the otherwise sound libsodium mode.

### Recommendation

Generate keys with libsodium `randombytes_buf`, Windows `BCryptGenRandom`, Linux `getrandom`, or Apple `SecRandomCopyBytes`. Fail closed if secure entropy is unavailable. Add a provider name to keygen output metadata and a self-test for RNG availability.

## VSA-003 — Bot session/challenge tokens use `Math.random()`

**Severity:** High  
**Affected component:** `examples/bot-detection/protected/bot-engine.js`

### Evidence

`makeOpaqueToken` hashes `Date.now()`, `Math.random()`, and session order to create session identifiers and nonces.

### Impact

These values are not cryptographically unpredictable. A hostile client can also instrument the protected runtime and observe state. The implementation provides ordering and freshness checks but should not be presented as cryptographic replay prevention or challenge binding.

### Recommendation

Generate 128–256 bits with a capability that ultimately uses `crypto.getRandomValues`. If QuickJS cannot access secure entropy directly, pass entropy through a narrowly scoped host capability and include it in the protected session derivation. Rename documentation to “client replay resistance” unless a server verifies the challenge.

## VSA-004 — Legacy `VSEAL001` decoders remain reachable

**Severity:** Medium  
**Affected components:** native and WASM package runtimes

### Evidence

Release inspection rejects legacy sections, but runtime parsers still recognize and decode `VSEAL001` payloads.

### Impact

Keeping deprecated formats increases parser and downgrade surface. A runtime invoked without the external release checker may accept a weaker envelope.

### Recommendation

Compile legacy support only in an explicit compatibility build. Production browser and native runtimes should reject the magic unconditionally. Add negative tests at the runtime layer, not only release inspection.

## VSA-005 — Dynamic libsodium loading trusts library search paths

**Severity:** Medium  
**Affected components:** `src/package/crypto.cpp`, `src/runtime/package_runtime.c`

### Evidence

Windows loads `libsodium.dll` or `sodium.dll` by name with `LoadLibraryA`; Unix uses common sonames with `dlopen`.

### Impact

In an unsafe application directory or altered search environment, a malicious library could be loaded into the compiler or native runtime process.

### Recommendation

Prefer static linking. Otherwise, resolve a configured absolute path and use safe Windows DLL search flags. Verify the loaded module path and optionally its digest/signature. Document trusted deployment requirements.

## VSA-006 — Package keys may persist in environment and global strings

**Severity:** Medium  
**Affected components:** package crypto override, CLI key loading, native runtime environment loading

### Evidence

Keys can be supplied through `VENOM_PACKAGE_KEY`/`VENOM_PACKAGE_KEY_HEX`. The C++ override is stored in a process-global `std::string`; clearing it does not explicitly wipe capacity before release.

### Impact

Keys may be exposed through child-process environments, crash dumps, diagnostics, process inspection, or stale heap memory.

### Recommendation

Prefer a restricted key file descriptor, OS key store, or secret provider. Wipe global buffers before clear, avoid inherited environment secrets, lock sensitive memory where supported, and never print key-bearing command lines.

## VSA-007 — Browser integrity lacks an immutable client trust anchor

**Severity:** Medium  
**Affected components:** generated HTML, loader, runtime, package and WASM binding

### Evidence

Hashes and binding values are delivered by the same origin as the assets they protect. SRI helps when HTML is trusted and assets are hosted separately, but an origin compromise can replace the HTML and all expected hashes together.

### Impact

Runtime integrity detects accidental mixing and partial tampering, not a fully compromised publisher origin.

### Recommendation

State this limitation explicitly. For stronger publisher authenticity, support signed manifests verified by a pinned public key in an independently trusted launcher, browser extension, native shell, or service worker installed from a trusted state.

## VSA-008 — CSP is not generated as an enforceable production profile

**Severity:** Medium  
**Affected components:** deployment documentation and generated output

### Evidence

Documentation recommends a tailored Content Security Policy, but the build does not emit a validated CSP configuration or test the dist under one by default.

### Impact

Deployments may omit CSP or enable broad `unsafe-inline`, blob, worker, or connect permissions. XSS would expose the bridge and application data regardless of bytecode protection.

### Recommendation

Generate a deployment-specific CSP template and add browser tests under that policy. Minimize `script-src`, `worker-src`, `connect-src`, `object-src`, `base-uri`, and `frame-ancestors`. Avoid `unsafe-eval`; migrate inline content to hashes/nonces where practical.

## VSA-009 — Native binary hardening is not explicitly enforced

**Severity:** Medium  
**Affected components:** CMake and release packaging

### Evidence

Warnings, sanitizers, IPO, CodeQL, and fuzz options exist, but release configuration does not centrally enforce or verify platform hardening flags.

### Impact

Compiler defaults vary. A packaged binary may miss stack protection, PIE/ASLR, RELRO, fortified libc, Control Flow Guard, or related mitigations.

### Recommendation

Add a `venom_apply_security_hardening` CMake function and a release-time binary inspection job. Treat missing expected mitigations as a release failure.

## VSA-010 — Vendored web dependencies lack an automated security inventory

**Severity:** Low  
**Affected components:** example vendor directories and release metadata

### Evidence

Examples include vendored JavaScript/CSS libraries, while CI does not produce a complete SBOM or fail on untracked vendored dependency changes.

### Impact

Old or modified libraries can persist unnoticed and may be copied into user projects.

### Recommendation

Generate CycloneDX or SPDX SBOMs, record upstream version and digest for each vendored asset, automate dependency review, and keep examples isolated from production runtime dependencies.

## VSA-011 — Security terminology can overstate browser guarantees

**Severity:** Low  
**Affected components:** metadata, console output, documentation

### Evidence

Terms such as “encrypted,” “AEAD,” “authentication,” and “secure runtime” appear around client-decodable mechanisms.

### Impact

Users may assume protection against an attacker who controls the browser, which is not achievable in the current architecture.

### Recommendation

Use precise labels: “protected bytecode,” “tamper-evident binding,” “browser-decodable envelope,” and “reverse-engineering resistance.” Reserve “authenticated encryption” for standard keyed cryptography.

## VSA-012 — Build diversification entropy is implementation-dependent

**Severity:** Low  
**Affected components:** polymorphic seed and binding-token generation

### Evidence

Several non-key values use `std::random_device`, sometimes mixed with time and address material.

### Impact

This is usually sufficient for build diversity but may produce repeatable or lower-entropy layouts on unusual platforms, reducing polymorphism.

### Recommendation

Use the same OS CSPRNG abstraction as native keygen. Keep deterministic seeds only behind explicit reproducible-build options and label deterministic artifacts clearly.

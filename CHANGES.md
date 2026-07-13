# Changelog

## 1.34.1

Repository consistency and release-maintenance cleanup:

- normalize the changelog and remove duplicated historical headings;
- replace stale `basic-site` test names with neutral site-preview or protected-chess names;
- update CLI examples to use `examples/protected-chess`;
- make release packaging tests read the project version dynamically instead of assuming `1.0.1`;
- repair the release packaging smoke test so it validates the single supported chess example.

## 1.34.0

Release-candidate repository cleanup:

- retain `examples/protected-chess` as the only supported public example;
- move compatibility and parser websites under `tests/fixtures/sites`;
- reduce scripts to canonical build, test, serve, analysis, readiness, and release entry points;
- remove historical hotfix, migration, maintenance, and generated-manifest debris;
- rewrite the root README around the current architecture and workflow;
- add a complete standalone README for the Protected Chess Lab;
- restore missing compiler build sources so clean checkouts configure and compile correctly.

## 1.33.0–1.33.6

Protection hardening series:

- add aggressive AST-aware JavaScript hardening for loader, worker, engine, and runtime assets;
- remove production extraction reports, source paths, stable candidate names, and descriptive WASM ABI symbols;
- add opaque per-build bridge envelopes, numeric export slots, session binding, and replay protection;
- move package section decode and decompression behind the WASM-owned boundary;
- diversify complete package section ordering and physical offsets per build;
- add mandatory production leak scanning and fail-closed release verification.

## 1.32.0–1.32.5

Protected bridge and chess integration series:

- introduce `venom.ready()`, `venom.call()`, `venom.exports`, and `venom.info()`;
- add bounded JSON transport, timeouts, cancellation, concurrency limits, and sanitized errors;
- extract the chess engine into protected QuickJS bytecode while retaining browser-native UI logic;
- fix bridge readiness ordering, export pre-registration, and QuickJS Promise result handling;
- modernize the chess example into the Venom Protected Chess Lab.

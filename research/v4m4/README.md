# V4-M4: verified SSA-like IR to build-specific bytecode

**Research-only, not production.** This milestone extends [V4-M3](../v4m3/README.md) with a verified typed integer IR, acyclic control-flow graphs, phi nodes and a capability-mediated host call.

## Architecture

```text
Typed uint32 Module
 → verify reachability, unique SSA definitions, dominance, phi completeness
 → lower CFG to logical instructions with branch-specific phi copy stubs
 → encode V4VMBC04 with per-build opcode/field layouts and checksum
 → decode/verify same-build envelope + operands
 → execute fuel-limited 16-register VM with capability policy
```

The independent IR evaluator and bytecode VM are differential-tested on **2,000 reproducible, seeded cases** using diamond control flow, join phis, uint32 arithmetic, and per-build opcode maps. Host-call tests verify default deny, explicit allowlist, callback presence, budget and no side effects on denied calls. Older V4-M3 regression tests also run.

## Build and test

```sh
cmake -S research/v4m4 -B build/v4m4
cmake --build build/v4m4 --config Release
ctest --test-dir build/v4m4 -C Release --output-on-failure
```

The V4-M4 revision changes the envelope to `V4VMBC04` and breaks compatibility with `V4VMBC03` intentionally, because the opcode set gained `HostCall`.

## Explicit limitations

- This is **not a JavaScript compiler**: it models uint32 literals, modulo-2^32 add/sub, bitwise XOR, branches, joins and mock host calls only. No objects, strings, exceptions, DOM, async, JS Number, `ToInt32`, or browser evaluation.
- Acyclic CFG only: back-edges are rejected. Values 1..15 map directly to 15 VM registers; no spilling or large-scale register allocation.
- No access to browser or network by default. The embedding application must explicitly provide an allowlist and callback; the example callback is a test stub.
- SHA-256 is an **unkeyed corruption check**, not a signature or authorization. All client-held bytecode maps can be observed by a browser debugger. Keep secrets, authorization and authoritative decisions server-side.
- The V4-M2 historical candidate was not available in the repository, so this is a newly defined compatible-in-spirit SSA-like bridge, **not** a verified import of the older V4-M2 implementation.

## Next steps

Expand the typed value representation and verify full JS language semantics against TurboJS/Test262 before browser integration; implement register allocation and loops, authenticated release packaging, host-side effect provenance and browser E2E tests. Do not replace production TurboJS or claim broad JS support until those gates pass.

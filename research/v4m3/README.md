# Venom V4-M3: build-bound polymorphic bytecode research

This is an **isolated, non-production research testbed** for the V4 compiler/runtime connection. The production Venom/TurboJS runtime remains unchanged.

* `vm.hpp/.cpp` define a seven-op semantic ISA, a deterministic per-build opcode map and word permutation, a compiler, strict decoder, and fuel-limited 16-register reference VM.
* The 64-byte `V4VMBC03` envelope includes a version, instruction count, 128-bit build identifier, and SHA-256 integrity digest for the header fields and encoded instruction payload.
* A different build's runtime must reject a bytecode package before execution. Invalid versions, lengths, operands and payload mutations fail closed.
* The tests cover determinism, diversity, roundtrips, execution, cross-build rejection, malformed bytecode, infinite loops, and 2,000 differential programs.

Build independently of the production project:

```sh
cmake -S research/v4m3 -B build/v4m3
cmake --build build/v4m3 --config Release
ctest --test-dir build/v4m3 -C Release --output-on-failure
```

## Security scope and remaining work

SHA-256 here detects accidental corruption and unsophisticated tampering; **it is not authenticated**, because the verifier and bytecode both live on an adversary-controlled client. Instruction masks and permutations hinder cross-build tooling, not dynamic instrumentation. Genuine secrecy and authorization require server-side enforcement and separately authenticated release artifacts.

This prototype intentionally implements integer operations only, **not** JavaScript semantics. Next integration stage: port the V4-M2 verified semantic IR lowering into this contract; introduce effectful host-call constraints, signed package binding, compatibility tests in the browser, and compiler/runtime reproducibility qualification on Windows and Linux. Until then, V4-M3 is a research demonstrator, not a shipped replacement for TurboJS.

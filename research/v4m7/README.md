# Venom V4-M7 — register-constrained SSA bridge (research)

This is an **experimental compiler milestone**, not part of the production browser runtime.

## Architecture

The V4-M5 JavaScript-subset entry point now executes this path:

```text
restricted JS source
  -> existing static parser and typed register-machine emitter
  -> V4-M6 definite-assignment / control-flow verifier
  -> V4-M7 SSA lifting: basic blocks, unique virtual definitions, 16
     register-state phi nodes at every reachable non-entry block
  -> V4-M7 SSA graph verification / structural and predecessor checks
  -> V4-M7 lowering with explicit branch targets
  -> re-run V4-M6 definite-assignment verifier
  -> unchanged V4VMBC05 per-build bytecode envelope
  -> existing fuel-limited V4 VM
```

`compile_source_direct` preserves the prior direct emitter solely for
research differential testing; `compile_source` routes through the SSA bridge.

The SSA graph has **unique virtual value IDs for every instruction write**
and explicit predecessor-defined values at loops and joins. The backend
**coalesces all phi values to their original physical VM register slot**,
so it currently requires only the original 16-register layout. The new
independent SSA interpreter can execute and verify the graph without calling
the production VM. Loop-header phi values are assigned in parallel.

**Important qualification:** This is not yet source AST → high-level typed
SSA. It is register-machine-bytecode → SSA lifting, then lowering back to
register-machine bytecode. The representation still carries original physical
register slots; it does not implement general register allocation, spilling
or optimization. The original static type validation remains in the frontend.

## Verification and constraints

- Reject malformed graph edges, detached CFG blocks, incomplete or
  duplicate phi inputs, missing/duplicate value definitions, forged operand
  origins, illegal writes to the immutable r0 sentinel and uninitialized
  physical register reads.
- Implicit initial virtual register values correspond to VM zero
  initialization; user programs cannot read an uninitialized register
  because the existing V4-M6 definite-assignment pass is still enforced.
- Stage phi incoming values before assigning phi results, including on
  loop backedges where two variables swap.
- Cap at 512 basic blocks, 80,000 SSA IDs and 65,536 output VM instructions,
  in addition to prior source/parser and execution-fuel limits.
- No bytecode envelope change: this backend emits the existing
  `V4VMBC05` ISA. Host capabilities remain default-deny and quota-limited.

## Tests

```sh
cmake -S research/v4m7 -B build/v4m7
cmake --build build/v4m7 --config Release
ctest --test-dir build/v4m7 -C Release --output-on-failure
```

The V4-M7 test target includes 1,400 seeded source/compiler-SSA/interpreter/
VM comparisons; path-sensitive control flow, loop-carried values, short-circuit
host calls, malformed SSA, overflow, fuel exhaustion and cross-build rejection.

V4 research CI separately builds/tests V4-M3 through V4-M7 on Linux/Clang
and Windows/MSVC and compares the existing 966-source program corpus against
Node 22. The broader product CI is not guaranteed by these focused tests.

## What remains

General JS Number semantics, strings, objects, functions, async/DOM APIs,
type-preserving frontend AST → typed SSA, actual register allocation/spilling,
browser E2E, binary signing and production release hardening remain separate
future work. A client-side opcode mapping and unkeyed SHA-256 checksum do not
make proprietary secrets inaccessible to someone controlling a browser.

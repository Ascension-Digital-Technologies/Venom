# Venom V4-M6: cyclic SSA IR and verified JS-subset control flow

**Research-only; not production integrated.** This milestone adds two complementary
control-flow verification paths without claiming they are yet one unified SSA compiler.

## Loop-aware SSA IR

- The V4-M4 IR verifier now accepts backedges and computes dominators using a
  fixed-point algorithm. Detached cycles, invalid phi predecessors, duplicate
  definitions and non-dominating SSA uses fail closed.
- Loop header phi values are resolved on incoming edges, including backedges.
  Phi copies execute **in parallel** using r15 as a scratch register to break
  cycles such as `a <- b, b <- a`. IR value IDs are consequently limited
  to 1..14. This is not a production register allocator.
- The existing independent IR reference evaluator provides differential
  comparisons with the bytecode VM. Dynamic loops remain fuel bounded.

## JavaScript subset integration

- The V4-M5 source frontend now executes `verify_compiled_cfg` on every
  generated program. The verifier validates edges, immutable r0, reaching
  definitions and all reads under branch merges and loop backedges.
- Strictly Boolean `&&` and `||` lower into **lazy** conditional branches,
  so right-side host effects never occur on a short-circuited path.
- The Node.js oracle now tests **966 source programs** including boolean
  expressions; new native V4-M6 tests execute **2,000 randomized loop/phi
  differential cases**, short-circuit effects and negative CFG examples.
- The V4-M5 bytecode format stays at `V4VMBC05`: new code reuses existing
  branch and move opcodes, with no wire-version change.

## Build and test

```sh
cmake -S research/v4m6 -B build/v4m6
cmake --build build/v4m6 --config Release
ctest --test-dir build/v4m6 -C Release --output-on-failure
```

V4-M3, V4-M4 and V4-M5 research regression builds also run separately in
`.github/workflows/v4-research.yml`. The Node 22 oracle runs on Linux and Windows.

## Important security and compatibility limits

This does **not** turn V4-M5 into an SSA-lowered JavaScript compiler.
V4-M5's structured frontend still directly lowers to verified VM bytecode;
the loop-aware SSA IR remains a separately validated research path. The
accepted JavaScript subset still does not support general Number semantics,
strings, objects, closures, async, DOM or full TypeScript. Runtime masks and
unkeyed checksums cannot hide secrets from a browser owner. Authorization
and proprietary data still belong on a trusted server.

Next: translate V4-M5's AST into the loop-capable IR, add tagged JS
values plus register spilling, then run browser equivalence certification.
Production-wide CI has separate pre-existing failures and is not claimed green.

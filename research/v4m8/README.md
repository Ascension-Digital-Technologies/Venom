# Venom V4-M8 — Source-direct typed SSA and last-use allocation

**Experimental research code only. Does not change production Venom or the default V4-M7 frontend.**

## What is actually new

A second, **opt-in** compiler frontend, `venom::v4m8::compile_source_typed`,
parses a strict, straight-line JavaScript subset directly into a typed,
immutable SSA node graph, without compiling bytecode and lifting it back into
SSA first. Each value has a unique virtual ID and a static `Number` or
`Boolean` type. Assigning a `let` variable changes the symbol's current
SSA ID, not the old SSA node.

The backend counts uses of each SSA value and frees physical VM registers
as soon as a value has been consumed for the last time. This lets a large
straight-line program with hundreds of virtual values execute within the
existing **15 usable physical VM registers** (r0 remains the branch sentinel).
When there are more than 15 **simultaneously live** values, compilation fails
closed with a register-pressure error. **There is no stack spilling yet.**

The graph has an independent reference evaluator, pre-lowering SSA
verification, and a version-compatible lowering to the existing
`V4VMBC05` bytecode VM.

## Accepted source

```js
function main() {
  const tax = 7;
  let subtotal = 20;
  subtotal = subtotal + 3;
  return (subtotal + tax) * 2;
}
```

One zero-argument `function main()`; function-scoped `let`/`const`
declarations with initializers and updates to `let`; exactly one final
`return`. Supports int32 literals, Boolean literals, parentheses, unary
`-`, `+`, `!`; checked int32 `+`, `-`, `*`; bitwise `^`; comparisons
`<`, `<=`, `>`, `>=` and strict `===`, `!==` with matching types.
Special opt-in `host(nonzero_literal_capability_id, numeric_arg)` is
still denied unless a trusted embedder supplies allowlist, callback and
finite call quota. It is not standard JavaScript.

`if`, `while`, short-circuit `&&`/`||`, loops, blocks, objects, strings,
array literals, general function calls, Number coercion, floating point,
closures and DOM APIs are **not implemented in this new frontend**. The
V4-M7 frontend remains available for its broader existing subset. This
milestone is a **source-direct typed SSA proof**, not yet a unified
AST-to-typed-SSA compiler for all accepted V4-M7 constructs.

## Pipeline

```text
JS source → bounded lexer and typed expression/declaration parser
  → typed SSA values and immutable assignment versions
  → SSA structural/type/forward-reference verification
  → use-count/last-use physical register allocation
  → logical VM instructions (including host capability checks)
  → existing V4VMBC05 build-specific encoder, decoder and VM
```

## Verification

```sh
cmake -S research/v4m8 -B build/v4m8
cmake --build build/v4m8 --config Release
ctest --test-dir build/v4m8 -C Release --output-on-failure
python3 research/v4m8/node_differential.py build/v4m8/venom_v4m8_js
# Windows: build/v4m8/Release/venom_v4m8_js.exe
```

The native test target includes **2,100 deterministic generated source
comparisons** among direct typed SSA interpretation, the new allocator/VM,
and the V4-M7 reference compiler; malformed SSA, host-policy and checked
overflow tests; plus a 400-reassignment case to verify register reuse.

The independent Node.js oracle checks **1,205 generated JS programs**
against the new typed VM. Dedicated research CI runs these tests with
earlier V4 milestones on both Linux/Clang and Windows/MSVC.

## Limitations and trust boundaries

The output is not a security proof. Checked signed-int32 operations throw
on overflow, unlike arbitrary JavaScript `Number`. Opcodes and mappings
inside a browser are inspectable; SHA-256 checksums without trusted signatures
do not authenticate packages. Server-side secrets, validation and access
control must stay server-side.

Next steps: direct typed SSA lowering for `if/else`, loop-carried phis and
short-circuit expressions, then virtual register spilling and browser
application equivalence. Only after parity can the new frontend replace
the V4-M7 default without risking compatibility regressions.

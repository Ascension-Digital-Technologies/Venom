# V4-M5: source JavaScript subset → build-specific VM (research)

**Experimental, not production integrated.** This milestone adds a parser/compiler for a **strictly specified subset** of JavaScript source. Unlike V4-M4's forward-only SSA IR, V4-M5 uses a separate structured-control-flow lowering path to support bounded loops. It does **not** claim that general JavaScript is compilable or that the V4-M4 SSA IR now supports back-edges.

## Supported source form

```js
function main() {
  const max = 5;
  let i = 0;
  let sum = 0;
  while (i < max) {
    sum = sum + i;
    i = i + 1;
  }
  if (sum === 10) { return sum; }
  else { return 0; }
}
```

* Exactly one zero-argument `function main()`; all control paths must return a Number or Boolean.
* Block-scoped `let` and `const` with explicit initializer, mutations of `let`, `return`, block statements, `if/else`, `while`, empty statements and comments.
* Decimal signed-int32-range literal values (literal `-n` compiles as checked unary negate), `true`/`false`, parentheses; arithmetic `+`, `-`, `*`; bitwise `^`; relational `<`, `<=`, `>`, `>=`; strict equality `===`, `!==`; unary `-`, `+`, `!`.
* No JavaScript implicit coercion: arithmetic and bitwise operations require Numbers; relational requires Numbers; strict equality requires matching types. The compiler rejects out-of-subset expressions instead of silently changing behavior.
* Numeric VM registers hold the bit representation of signed 32-bit integers. Checked `+`, `-`, `*` **throw on signed overflow** instead of returning a wrong JavaScript result. This is a restricted safe-integer domain, *not* arbitrary JavaScript `Number` semantics. Bitwise XOR matches ToInt32 for supported values. The command-line tool prints the `uint32` bit pattern, so `-1` prints `4294967295`.
* The **Venom-only** `host(nonzero_literal_capability_id, number_arg)` intrinsic compiles to `HostCall`. It is not standard JavaScript and does not grant ambient APIs. Runtime calls require both a matching allowlist and a host callback, with a finite per-execution budget; all calls are denied by default.

## Pipeline and limits

```text
JS subset source → bounded lexer/parser + static type/scope checks
  → structured code emission + 16-register local/temporary allocation
  → logical bytecode (checked arithmetic, branches, fuel-limited while)
  → per-build opcode and field permutation
  → V4VMBC05 envelope + SHA-256 corruption check
  → same-build strict decoder / logical VM interpreter
```

There are 15 allocatable user registers (register zero is an unconditional-branch sentinel). Scope depth is capped at 32, expression and unary depth at 64, the token count at 8,192, source size at 65,536 bytes, and VM instructions at 65,536. Control-flow loops are limited dynamically by the VM's execution fuel. There is no heap, DOM, network, dynamic import, `eval`, array/object access, string, async, closures, short circuit operators, general function calls, implicit coercion, lexical shadowing, or exception handling. Unsupported source is rejected.

## Test commands

```sh
cmake -S research/v4m5 -B build/v4m5 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v4m5 --config Release
ctest --test-dir build/v4m5 -C Release --output-on-failure
python3 research/v4m5/node_differential.py build/v4m5/venom_v4m5_js
# On Windows, use build/v4m5/Release/venom_v4m5_js.exe
```

`tests.cpp` checks 1,000 seeded loop executions, semantic cases, build isolation, tamper checks, overflow, control-flow fuel, compile-time rejections, host policy denial, quotas and recursion limits. `node_differential.py` compares **726 generated source functions against Node.js** for the accepted integer and Boolean grammar; it only evaluates its own generated test corpus. The test never enables arbitrary browser or network capabilities in the VM.

## Trust boundaries and next integration step

The per-build mapping and an unkeyed SHA-256 checksum are *not* secrets, tamper-proofing or authorization: an adversary with control of a client can inspect the VM and change bytes/checksums. For production, integrate signing, server-backed authorization, browser equivalence, typed/full-JavaScript IR, certified compatibility on real apps, and a unified loop-capable SSA verifier before replacing TurboJS. **Do not merge this research-only runtime into the shipped browser engine as if it supported full JavaScript.**

# Compiler diagnostics

Venom compiler diagnostics use stable identifiers so build systems and documentation can refer to a failure independently of its wording.

A source-aware diagnostic contains:

- a stable `VENOM-E####` code;
- the failing compiler phase;
- a source file, line, and column when available;
- the relevant source line and caret;
- technical detail from the parser or planner;
- an actionable remediation; and
- a documentation reference.

## VENOM-E2101

**JavaScript parse failed.** Correct the syntax at the reported location. Venom parses source before planning or rewriting it, so malformed input fails before a protected distribution is generated.

## VENOM-E2102

**JavaScript scope analysis failed.** Simplify the affected declaration or move runtime-bound behavior behind an explicit capability.

## VENOM-E2202

**Unsupported protected-module export.** Protected modules currently expose named function declarations only.

## VENOM-E2205

**Dynamic import in a protected module.** Move the dynamic import boundary to browser code or replace it with a statically resolvable module dependency.

## VENOM-E2301

**Protected-function lowering failed.** Use a supported named function, function expression, or variable-bound arrow declaration.

## VENOM-E2303

**Generator protected functions are unsupported.** Convert the generator to a normal or async function before protecting it.

## VENOM-E2304

**Ambiguous protected variable declaration.** A protected variable declaration must contain exactly one binding so Venom can replace the complete declaration safely.

## VENOM-E2401

**Malformed protected bridge contract.** Use comma-separated `name:type` fields after `@venom: input` or `@venom: output`.

## VENOM-E2402

**Unsupported protected bridge contract type.** Use one of the documented scalar, object, array, or typed-array field forms.

## VENOM-E2403

**Duplicate protected bridge contract field.** Each input or output field name must appear only once.

## Automation

Diagnostic codes are stable within a major release. Human-readable wording and remediation detail may improve in minor releases without changing the code.

## VENOM-E2501

**TypeScript transpilation failed.** Correct the unterminated or malformed type-only declaration at the reported location.

## VENOM-E2502

**Unsupported TypeScript runtime construct.** Replace `enum` or `namespace` with ordinary JavaScript, or precompile it before Venom.

## VENOM-E2503

**JSX lowering is not enabled.** Precompile JSX with the framework toolchain or keep the JSX-bearing file in the browser build pipeline.

## VENOM-E2504

**External structural TypeScript lowering failed.** This code is retained only for the explicit legacy `node` frontend during the parity transition. Use the default embedded frontend or correct the external frontend environment.

## VENOM-E2505

**TypeScript compilation failed.** Correct the TypeScript syntax at the reported source location. The diagnostic detail includes the original `TS####` compiler code and message returned by the embedded TypeScript compiler.

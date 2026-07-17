# TypeScript input

Venom 1.1 accepts `.ts`, `.mts`, `.cts`, and `.tsx` files as compiler inputs. TypeScript is erased in memory before the existing JavaScript AST, scope, lowering, contract, and QuickJS bytecode phases run.

No Node.js process or external TypeScript compiler is required for this path.

## Supported type-only syntax

The native frontend currently erases common TypeScript forms while preserving line structure for diagnostics:

- `interface` declarations;
- `type` aliases;
- `import type` and `export type` statements;
- parameter, variable, and return-type annotations;
- optional parameter markers;
- `as Type` and `satisfies Type` assertions;
- non-null assertions;
- declaration-only modifiers such as `public`, `private`, `protected`, `readonly`, `abstract`, and `override`.

Relative module resolution supports extensionless TypeScript imports and JavaScript-style specifiers that resolve to local `.ts`, `.tsx`, or `.mts` source files.

```typescript
// @venom: protected module
import type { ScoreInput, ScoreResult } from "./types";
import { calculate } from "./math";

// @venom: input value:number
// @venom: output result:number
export async function score(input: ScoreInput): Promise<ScoreResult> {
  return { result: calculate(input.value) } as ScoreResult;
}
```

## Runtime TypeScript constructs

The native frontend is intentionally a deterministic type eraser, not a replacement for every TypeScript transformation. Constructs that emit additional JavaScript, including `enum` and `namespace`, fail closed with `VENOM-E2502`. Precompile those constructs before Venom or express them as ordinary JavaScript.

## TSX and JSX

`.tsx` files are accepted when they contain TypeScript without JSX. JSX syntax currently fails with `VENOM-E2503`; framework projects should run their normal JSX transform first and pass the resulting JavaScript or non-JSX TypeScript to Venom.

## Diagnostics and source locations

Erased syntax is replaced with whitespace rather than deleting line content. This keeps parser and lowering diagnostics aligned with the original TypeScript file. The private project IR records whether each module was transpiled and how many declarations, annotations, and assertions were erased.

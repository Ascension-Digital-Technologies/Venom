# AST symbol-level module closure

Venom resolves static ES module imports to the exact exported function used by a planned function. A module may therefore expose both protected-compatible and browser-bound functions without forcing every named importer into the browser runtime.

## Rules

- Named imports propagate the runtime of the referenced exported function.
- Aliases preserve the original imported symbol identity.
- Namespace imports remain whole-module boundaries because property access is dynamic.
- Side-effect imports retain whole-module behavior.
- Missing exports, unresolved relative modules, and external package boundaries require review.
- Explicit annotations and planner overrides remain authoritative.

## Example

```js
// mixed.js
export function calculate(value) { return value * 2; }
export function renderTitle() { return document.title; }
```

```js
import { calculate } from "./mixed.js";
export function quote(value) { return calculate(value); }
```

`quote` remains protected even though `mixed.js` also exports browser-bound logic. Importing `renderTitle` propagates the browser runtime.

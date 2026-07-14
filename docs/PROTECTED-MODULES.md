# Protected module graph

Protected modules let a website move a private JavaScript module graph into the QuickJS/WASM execution boundary while preserving normal browser imports for selected entry points.

## Basic graph

```javascript
// evaluation.js
// @venom: protected module

const materialWeight = 10;
function normalize(value) { return Number(value) || 0; }

export function evaluate(position) {
  return normalize(position.material) * materialWeight;
}
```

```javascript
// engine.js
// @venom: protected module

import { evaluate } from "./evaluation.js";

function normalize(input) { return input || {}; }

export function search(input) {
  const request = normalize(input);
  return { score: evaluate(request.position) };
}
```

```javascript
// browser.js
import { search } from "./engine.js";

const result = await search({ position: { material: 4 } });
```

`evaluation.js` and `engine.js` receive independent lexical scopes. Their two private `normalize` declarations cannot collide. Because `evaluation.js` is imported by another protected module, `evaluate` remains internal and is not added to `venom.exports`.

## Supported imports

Named imports:

```javascript
import { evaluate, orderMoves as order } from "./search-core.js";
```

Namespace imports:

```javascript
import * as core from "./search-core.js";
```

Imports must be relative and resolve to another file marked `// @venom: protected module`.

## Supported exports

```javascript
export function calculate(input) {}
export async function calculateAsync(input) {}
```

Public entry-module exports become generated browser facades and are also callable through `venom.exports`.

## Fail-closed diagnostics

| Code | Meaning |
|---|---|
| `VENOM-E2202` | Unsupported export form |
| `VENOM-E2203` | No named function exports |
| `VENOM-E2204` | Duplicate public export name |
| `VENOM-E2205` | Dynamic import is unsupported |
| `VENOM-E2206` | Side-effect-only import is unsupported |
| `VENOM-E2207` | Malformed import statement |
| `VENOM-E2208`–`E2210` | Malformed exported function syntax |
| `VENOM-E2211` | Import is not relative |
| `VENOM-E2212` | Protected dependency cannot be resolved |
| `VENOM-E2213`–`E2215` | Unsupported or malformed import clause |
| `VENOM-E2216` | Protected module cycle detected |

Venom never silently moves an invalid protected module back into browser execution.

## Current constraints

Default imports, side-effect-only imports, dynamic imports, re-exports, exported variables, exported classes, default exports, and top-level await are not part of the v1 module contract. The browser bridge remains JSON-only and asynchronous.

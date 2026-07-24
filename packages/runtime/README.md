# @venom/runtime

Stable browser SDK for Venom protected TurboJS/WASM applications.

```ts
import { initializeVenom, callProtected } from "@venom/runtime";

await initializeVenom();
const result = await callProtected<{ values: number[] }, { score: number }>(
  "calculateScore",
  { values: [4, 8, 15, 16, 23, 42] }
);
```

The SDK normalizes bridge failures into `VenomRuntimeError`, supports abort signals and timeouts, reports runtime lifecycle state, validates protected exports before use, and exposes explicit disposal.

# @venom/react

React hooks for applications protected by Venom.

```tsx
import { useProtectedCall, useVenomRuntime } from "@venom/react";

function QuoteButton() {
  const runtime = useVenomRuntime();
  const quote = useProtectedCall<{ seats: number }, { total: number }>("calculateQuote");

  return (
    <button disabled={!runtime.ready || quote.pending} onClick={() => quote.execute({ seats: 5 })}>
      {quote.pending ? "Calculating…" : "Calculate protected quote"}
    </button>
  );
}
```

The hooks are Strict Mode safe, cancel superseded calls, and surface structured `VenomRuntimeError` failures from `@venom/runtime`.

# Framework qualification matrix

> Generated from checked-in browser-equivalence manifests. “Qualified pattern” validates representative production output patterns; it is not a blanket guarantee for every framework release or plugin.

| Framework | Version/output pattern | Delivery | Tier | Evidence | Scenarios |
|---|---|---|---|---|---:|
| React | 16.0.0 / ReactDOM 16.0.1 | production UMD | probe | behavioral | 1 |
| React | 18-compatible output pattern | Vite-style production ESM | qualified-pattern | behavioral | 1 |
| Vue | 3-compatible output pattern | Vite-style production ESM | qualified-pattern | behavioral | 1 |
| Svelte | 5-compatible compiled output pattern | Vite-style production ESM | qualified-pattern | behavioral | 1 |

## Interpretation

- **probe**: a focused compatibility signal.
- **qualified-pattern**: source and protected builds are compared in real browsers for a representative production output pattern.
- **qualified**: reserved for a pinned upstream application and broader scenario coverage.

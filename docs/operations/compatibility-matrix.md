# Compatibility support matrix

| Area | Status | Guidance |
|---|---|---|
| Static HTML/CSS/assets | Strong | Standard workflow |
| Classic JavaScript | Strong with analysis | Mark DOM-heavy functions browser-side |
| ES modules | Supported subset | Qualify static/dynamic imports |
| Multi-page routes | Supported | Verify route and asset paths |
| SPA routing | Supported with hosting config | Exclude assets from fallback rewrites |
| Timers/fetch/events | Capability dependent | Review compatibility report |
| React/Vite output pattern | Qualified pattern | Renderer browser-side; protect JSON-safe business logic |
| Vue/Vite output pattern | Qualified pattern | Keep proxies/templates browser-side; pass plain snapshots |
| Svelte/Vite output pattern | Qualified pattern | Keep compiled DOM updates browser-side; protect pure logic |
| Dynamic `eval`/`new Function` | Not a protected production strategy | Refactor or keep required code browser-side |
| DOM objects across bridge | Unsupported | Pass JSON-safe data only |


See [framework qualification](framework-qualification.md) for the generated evidence matrix and [framework guidance](framework-guidance.md) for integration boundaries.

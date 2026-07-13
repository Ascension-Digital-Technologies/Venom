# Framework compatibility

Venom treats framework support as an evidence claim, not a consequence of compiling a bundle. A framework/version combination is promoted only after its pinned production artifact compiles, boots, renders, and passes declared interactions in the real-browser matrix.

## React 16 UMD probe

Version 1.9.0 added an exact, pinned React 16 production UMD fixture:

- React 16.0.0 production UMD
- ReactDOM 16.0.1 production UMD
- class-component rendering
- state initialization
- synthetic click handling
- state-driven DOM update

The fixture is located at `examples/react16-umd-site`. Its browser evidence is generated independently for Chromium, Firefox, and WebKit and is bound to both the fixture manifest and the generated Venom distribution.

## Claim boundary

A passing report supports only the exact versions, delivery format, and behavior listed above. It does not establish support for modern React, hooks, JSX transformation, React Router, Server Components, hydration, Next.js, or arbitrary third-party React packages. Those require separate pinned fixtures.

A compile-only result is not framework support. Until real-browser evidence passes in CI, the fixture remains a compatibility probe rather than a published support guarantee.

## UMD compatibility preflight

Production UMD bundles commonly contain a guarded CommonJS branch such as `typeof exports === "object" ? require(...) : window.Library`. Venom 1.9.0 recognizes this pattern and reports it as `javascript.umd-commonjs-branch` with `partial` status rather than treating the unreachable browser-side `require` branch as an unconditional Node.js dependency. Real-browser validation remains required because static scanning cannot prove which branch executes.


See [Framework support policy](framework-support-policy.md) for the machine-enforced probe, candidate, and supported tiers.

# v0.93.0 compatibility and correctness expansion

v0.93.0 adds a production compatibility corpus that exercises the compiler and protected runtime against a multi-route website rather than a single synthetic page.

The corpus covers nested HTML routes, classic scripts, ES modules with transitive imports, promises and async fetch, timers, forms and events, JSON assets, SVG assets, CSS `@import`, CSS `url(...)` rewriting, relative paths from nested routes, protected route-shell collapse, and the minimal production distribution contract.

The release gate builds the same corpus in `debug` and `browser-protect` profiles. Debug output must preserve route shells for inspection. Protected output must contain only `index.html` and the hashed `assets/` tree, pass `release-check`, and verify the real QuickJS/WASM engine.

# Routing and route hydration

Venom compiles routes and HTML structure into a compact runtime representation. The route runtime reconstructs application structure while preserving browser-side navigation and asset behavior.

The compiler includes malformed-HTML and optional-end-tag regressions for elements such as `p`, `li`, `tr`, `td`, and `option`. Route bytecode is validated to avoid synthetic-root stack underflow.

For SPAs, configure server fallback for application paths while excluding `/assets/`.

# TypeScript compiler frontend

Venom vendors the TypeScript compiler API solely for deterministic structural lowering of `.ts`, `.tsx`, `.mts`, and `.cts` sources. The compiler is invoked through `tools/typescript_structural_frontend.js`; no package installation or network access is performed during a build.

The upstream license is preserved in `LICENSE.txt`, and package provenance is recorded in `package.json`.

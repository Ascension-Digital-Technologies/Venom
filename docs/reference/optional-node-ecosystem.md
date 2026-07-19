# Optional Node ecosystem integrations

The Venom compiler, embedded TypeScript frontend, native build, package verifier,
and core release closure do not require Node.js.

Node.js is used only when explicitly working with Node-native ecosystems:

- `packages/vite/`
- `packages/runtime/` npm lifecycle tests
- browser-equivalence harnesses implemented as Node test programs
- optional runtime/WASM diagnostic probes

These checks are disabled by default. Enable them with:

```sh
cmake -S . -B build -DVENOM_ENABLE_NODE_ECOSYSTEM_TESTS=ON
```

Core product code must not add a Node subprocess or `VENOM_NODE` dependency.

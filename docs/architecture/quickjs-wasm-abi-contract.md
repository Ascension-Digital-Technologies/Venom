# QuickJS/WASM ABI Contract

`contracts/quickjs-wasm-abi.json` is the authoritative browser runtime ABI.

The generator `tools/generate_quickjs_wasm_abi.py` produces:

- `include/venom/generated/contracts/quickjs_wasm_abi.hpp`
- `src/generated/runtime/javascript/quickjs_wasm_abi.js`
- `docs/generated/quickjs-wasm-abi.md`

Before a site build begins, Venom parses the export section of the embedded WASM bytes and verifies every required export from the generated C++ contract. Provenance text is not accepted as a substitute for the actual WebAssembly export table.

Development and release use the same WASM ABI. They differ only in accepted bytecode trust domains. Playground executions use a disposable QuickJS context and are qualified in real browsers through `tests/browser/venom_examples_e2e.py`.

# Dynamic module graphs

Venom 1.30.0 discovers and executes protected package-local ES module graphs.

The compiler recognizes static imports, `export ... from`, literal `import()` expressions, and HTML `modulepreload` hints. Entry modules with dependencies are encoded as `VQJSMB04` bundles containing named `VQJSBC03` native QuickJS bytecode records. The runtime validates every nested record, registers dependency modules in memory, evaluates the entry module, and drains QuickJS pending jobs. No public JavaScript source chunk is emitted.

Non-literal dynamic imports remain unsupported because their complete chunk set cannot be proven at build time. Remote module source is not fetched dynamically by the protected runtime.

## Runtime synchronization

`VQJSMB04` support is part of the QuickJS/WASM implementation, while the public execution ABI remains unchanged. The compiler refuses a protected module-graph build unless embedded runtime provenance declares `module_bundle_contract=VQJSMB04` and `literal_dynamic_import=true`. Rebuild the canonical runtime with `scripts/build-quickjs-wasm` after updating the runtime source.

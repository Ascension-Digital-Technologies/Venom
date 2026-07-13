# Product overview

## Accurate product description

Venom is a protected web compiler and dual-engine browser runtime. It transforms supported static and client-rendered websites into compact distributions containing polymorphic route/DOM bytecode, stripped native QuickJS bytecode, content-addressed runtime assets, and an integrity-checked application package.

Venom uses two coordinated execution layers:

- a custom Route VM interpreter in the generated browser runtime for route selection, packaged page reconstruction, and deterministic DOM construction; and
- a real QuickJS engine compiled to WebAssembly for JavaScript language execution.

QuickJS reaches supported browser functionality through Venom's host bridge. The bridge exposes selected APIs and enforces implemented validation, limits, and policy checks.

## What “protected” means

Venom aims to:

- remove immediately readable application source from normal protected output;
- compile JavaScript into native QuickJS object bytecode with source and debug records stripped;
- compile route and deterministic DOM structure into a custom bytecode format;
- vary protected package structure and Route VM encoding between builds;
- detect malformed packages and inconsistent runtime assets;
- fail closed when the real QuickJS/WASM engine is unavailable or invalid;
- prevent silent execution through readable host JavaScript in production mode.

Protection is resistance, not secrecy. A determined user controls the browser, workers, network, storage, debugger, WebAssembly memory, and downloaded artifacts. Venom must not be used to store browser secrets or replace server-side authorization.

## What Venom is not

Venom is not:

- a secure enclave;
- a tamper-proof browser;
- an unextractable client-side key store;
- a complete browser API implementation;
- a universal web-application compiler;
- a guarantee that application behavior is identical to direct browser execution;
- a substitute for release signing, HTTPS, CSP, or server security.

## Recommended short descriptions

### GitHub description

> Protected web compiler and dual-engine browser runtime using polymorphic route bytecode and real QuickJS bytecode executed in WebAssembly.

### Website headline

> Protect and package web applications with polymorphic route bytecode and QuickJS running in WebAssembly.

### Release description

> Venom compiles supported websites into polymorphic route/DOM bytecode and stripped native QuickJS bytecode, delivered through a fail-closed, integrity-checked browser runtime.

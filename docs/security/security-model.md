# Security model

Venom raises the cost of recovering and modifying valuable client logic by combining representation change, isolation, diversification, integrity binding, and release enforcement.

## Security benefits

- protected code is absent from ordinary browser JavaScript;
- QuickJS bytecode requires engine-aware recovery;
- package decoding crosses a WebAssembly boundary;
- dedicated workers separate UI and protected execution;
- production export slots and bridge identifiers are opaque;
- build-specific layout frustrates reusable extraction scripts;
- bound assets detect mismatched loader/runtime/package combinations;
- production gates scan for source, ABI, decoder, report, and metadata leakage.

Use Venom alongside HTTPS, CSP, server authorization, secure key management, and normal application security.

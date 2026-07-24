# Core release closure

Venom has one authoritative repository-level release command:

```bat
scripts\windows\verify-release.bat
```

```bash
scripts/linux/verify-release.sh
```

When called without a release archive, the command builds the native compiler and runs the core release closure. The closure verifies:

- the package, runtime, TurboJS/WASM, host bridge, and bytecode ABI lock;
- generated contract bindings;
- the embedded Node-free TypeScript frontend boundary;
- the TurboJS/WASM export and generated-engine contracts;
- build-bound and pre-bytecode hardening boundaries;
- production package metadata and release gates;
- release entrypoint policy.

A machine-readable report is written to `artifacts/core-release-closure.json`.

To verify an already packaged release directory or archive, pass its path:

```bat
scripts\windows\verify-release.bat release\venom-2.0.0-windows-x64.zip --expect-version 2.0.0
```

ABI-affecting changes require an intentional lock update:

```bash
python tools/verify_contract_lock.py --update
```

The updated `contracts/abi-lock.json` must be committed with the implementation and generated bindings.

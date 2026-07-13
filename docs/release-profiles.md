# Release profiles

Venom has two production-oriented protection targets. They intentionally make different promises.

## `browser-protect`

Use this for ordinary static web-server output where `index.html` and `assets/` are served to browsers.

Properties:

- package sections use the browser-runnable `VAEAD001` envelope
- readable internal section names are aliased
- debug manifests and source-preserving JavaScript records are rejected by `release-check`
- the loader, runtime, style, QuickJS engine shim, and QuickJS WASM asset are package-bound by hash
- package layout is polymorphic per build
- route/script chunks are opened lazily
- `release-profile.vrpf` declares `threat_model=browser-client-protection-v1`
- `release-profile.vrpf` declares `secret_material_model=browser-runtime-no-hidden-secret`

Limit: browser-protect cannot protect server secrets. Anything required to execute in the browser can be instrumented by a determined user.

## `native-protect`

Use this for private/native runtime paths where a package key can remain outside the distributed package.

Properties:

- package sections use `VSODIUM1`
- `VSODIUM1` is backed by libsodium XChaCha20-Poly1305-IETF
- the package requires a 32-byte key from `--key-file` or process environment
- browser JS/WASM intentionally rejects `VSODIUM1` packages
- `release-check --require-audited-crypto` fails if non-libsodium sections are present
- `release-profile.vrpf` declares `threat_model=native-private-aead-v1`
- `release-profile.vrpf` declares `secret_material_model=external-32-byte-package-key-required`

## Required release-check gates

A production artifact should pass:

```bash
venom release-check dist-browser --target browser
venom release-check dist-native --target native --key-file venom.key --require-audited-crypto
```

Release-check fails on:

- raw source/debug markers
- external `asset-manifest.txt` in protected output
- readable internal section names
- legacy `VSEAL001` envelopes
- missing package binding
- missing release profile metadata
- missing polymorphic layout metadata
- missing lazy-section metadata
- missing runtime hardening/integrity flags
- wrong browser/native crypto provider for the selected target


## v0.48 QuickJS/WASM execution gate

`browser-protect` now defaults to `--quickjs-backend wasm-real` and denies host fallback. Release-check requires the encrypted `quickjs-wasm-execution.vqwe` contract and reports `quickjs_wasm_execution`, `quickjs_execution_backend`, `quickjs_host_js_fallback_allowed`, `quickjs_release_fail_closed`, and `quickjs_wasm_chunks`.


## v0.49 WASM-owned bytecode boundary

Protected browser and native release profiles now require the QuickJS bytecode boundary to hand `VQJSBC03` native object records to the WASM runtime as opaque bytes. The generic browser runtime parser must not decode protected bytecode records back into JavaScript source. `release-check` reports `quickjs_bytecode_boundary: wasm-owned`, `quickjs_source_transfer: opaque-vqjsbc03-native-object`, and `quickjs_execute_bytecode_export: yes` when this boundary is active.


## v0.52 QuickJS/WASM semantic runtime

Protected browser and native release profiles now require the QuickJS/WASM runtime to satisfy the ABI12 boundary. `release-check` reports `quickjs_runtime_abi: 12`, `quickjs_abi_contract: quickjs-wasm-abi12-runtime`, `quickjs_abi12_runtime: yes`, `quickjs_status_exports: yes`, `quickjs_limit_exports: yes`, `quickjs_bytecode_validate_export: yes`, and `quickjs_backend_contract_export: yes` when the stronger runtime contract is active.


## v0.59.0 bytecode semantics note

Protected builds now require upstream bytecode-semantics exports in addition to the object/exception/module runtime bridge exports. See `docs/release-profiles.md`.

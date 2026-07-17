# Package format notes

> **Current interpretation:** This document includes historical format evolution notes. For security terminology, browser-runnable `VAEAD001` sections are runtime-decodable authenticated encoding, not confidential encryption against the client. External-key libsodium packages are limited to native/private workflows.

## v0.41 audited libsodium provider

v0.41 adds an optional `VSODIUM1` protected-section envelope selected with `--crypto-provider libsodium`. This path uses libsodium `crypto_aead_xchacha20poly1305_ietf_encrypt/decrypt`, a random 24-byte nonce per section, associated data bound to the envelope version, section type, stored-name hash, decoded payload size, and stored section name, plus a 32-byte package key from `VENOM_PACKAGE_KEY` / `VENOM_PACKAGE_KEY_HEX`. C++ package tools and the native C runtime can open these packages when libsodium and the same key are available. Browser JS/WASM runtimes reject `VSODIUM1` packages fail-closed.

## v0.40 AEAD section envelope

v0.40 protect/release packages use a `VAEAD001` section envelope for protected payloads. Payloads are compressed first, then wrapped with a version, section type, stored-name hash, payload size, nonce pair, and 128-bit authenticated tag before the transformed bytes. Readers verify the envelope before decompression and fail closed on type, name, size, or tag mismatch.

## v0.39 protected section aliases

v0.39 keeps debug package names readable, but protect/release packages can redact every non-public-asset section name into a deterministic `s.<fnv64>` alias. The runtime readers resolve expected canonical names through this alias map before decoding sections. This removes obvious internal filenames from the VBC name table without changing section type IDs, payload offsets, or compression behavior.

## v0.37 package feature table

v0.37 adds `package-features.vfeat` as `SectionType::PackageFeatures` (`88`). The 32-bit package flags are now treated as coarse profile/runtime metadata only. Fine-grained QuickJS, host bridge, runtime policy, module, replay, snapshot, and host-I/O capabilities are represented as table records with section type, section name, release-required status, and decoded-section SHA-256 digest. Native C++, browser JavaScript, and the C runtime all know this section type; release packages require the feature table before execution.

The table is intentionally not a signing claim. Integrity remains `sha256-software-v1`; protected payloads use either the browser-runnable `venom-aead-section-v1` envelope or the audited native `libsodium-xchacha20poly1305-ietf-v1` provider, and package signing remains an explicit future provider.

# VBC Package Format

Venom v0.40 uses a structured binary package with stable offsets, a fixed section table, a separate name table, payload offsets, package-level validation, per-section stored-payload hashes, decoded-section SHA-256 integrity metadata, and protected-profile section ordering hooks.

## Header

All integers are little-endian.

```text
magic                 u8[8]  "VENOMVBC"
version               u32    current: 40
runtime_abi           u32    current: 1
flags                 u32    profile/build flags
section_count         u32
section_table_offset  u64    currently 80
section_table_size    u64    section_count * 48
name_table_offset     u64
name_table_size       u64
payload_offset        u64
payload_size          u64
package_hash          u64    FNV-1a64 over the package image with this field zeroed
```

## Section entry

Each section table entry is fixed-size, so the reader can inspect all metadata before loading payload contents.

```text
type                  u32
flags                 u32
name_offset           u32    relative to name_table_offset
name_size             u32
data_offset           u64    absolute package offset
data_size             u64
raw_size              u64    uncompressed section size
hash                  u64    FNV-1a64 over stored section bytes
```


## Section compression

Production packages use real section-level compression. A compressed section has `SectionFlagCompressed` set, `data_size` equal to the stored compressed payload size, and `raw_size` equal to the decoded payload size. Hashes are computed over the stored bytes before decompression.

The current codec is a dependency-free LZ-style stream with magic `VCLZ0008`. It is intentionally isolated behind `src/package/compress.*` so a stronger production codec can replace it without changing the package table layout.

Debug builds leave sections uncompressed for readability. Protect/release builds compress only when the compressed payload is smaller than the original.

## Integrity metadata

Production builds include an integrity section whose canonical name is `integrity-auth.vsig`; in protected packages the stored name is an opaque alias. It is deliberately text-readable for inspection and covers the decoded section payloads that the runtime actually consumes.

```text
VENOM_INTEGRITY_V1
provider=sha256-software-v1
scope=decoded-sections
aead_provider=<venom-aead-section-v1|libsodium-xchacha20poly1305-ietf-v1>
profile=<debug|protect|release>
section_count=<n>
section	<section_type>	<section_name>	<decoded_size>	<sha256_hex>
```

The older 64-bit FNV table hashes remain fast structural checks over stored payload bytes. The SHA-256 metadata is the stronger execution boundary after decompression. `prod` packages fail closed when this metadata is missing or mismatched.

`aead_provider=venom-aead-section-v1` identifies Venom's browser-runnable, runtime-decodable authenticated encoding layer. It raises casual extraction cost and validates envelope consistency, but it is not an audited secret-key confidentiality boundary against the browser operator. `aead_provider=libsodium-xchacha20poly1305-ietf-v1` identifies the optional native/private provider using an external key; browser runtimes reject those packages.

## Section types

```text
1  manifest
2  routes
3  dom_templates
4  css
5  javascript
6  quickjs
7  vm_bytecode
8  asset
9  integrity
10 strings
11 asset_manifest
12 host_bridge
```

## Host bridge metadata

`host-calls.vhcb` is a readable `host_bridge` section with magic `VENOM_HOST_BRIDGE_V2`. It records the current runtime mode, stable host-call IDs, inline-event binding policy, and enabled fetch metadata plus planned QuickJS bridge slots. This is metadata for runtime validation and diagnostics; it is not encryption.

## Route sections

The HTML route compiler emits three binary sections plus one diagnostic preview:

```text
routes.vbrt
  magic                 u8[8]  "VRTE0003"
  version               u32
  route_count           u32
  repeated route entries:
    route_string_id     u32
    source_string_id    u32
    bytecode_offset     u32    offset into encoded instruction stream
    bytecode_size       u32
    instruction_count   u32
    reserved            u32

strings.vstr
  magic                 u8[8]  "VSTR0011"
  version               u32
  string_count          u32
  repeated string spans:
    offset              u32
    size                u32
  raw UTF-8 string data

route-bytecode.vmbc
  magic                 u8[8]  "VBCODE03"
  version               u32
  instruction_size      u32    currently 16 bytes
  instruction_count     u32
  reserved              u32
  encoded instruction stream
```

## Polymorphic VM metadata

`vm-polymorph.vpol` carries the decoder contract for the current package:

```text
VENOM_POLYMORPH 8
seed=<u32>
enabled=<0|1>
instruction_size=16|20|24|28
word_layout=<opcode|a|b|c>,<opcode|a|b|c>,<opcode|a|b|c>,<opcode|a|b|c>
opcode_xor=<u32>
operand_xor=<u32>,<u32>,<u32>
shuffle_strings=<0|1>
shuffle_routes=<0|1>
host_call dom=<u16> event=<u16> fetch=<u16> quickjs=<u16>
opcode_map:
  <logical> => <physical>
```

Debug builds use identity-style deterministic encoding. Protect/release builds randomize physical opcodes, instruction word order, decode masks, string order, route order, and package section order.

## JavaScript bundle section

`scripts.vjsb` is a binary ordered script bundle emitted by the bridge:

```text
magic                 u8[8]  "VJSB0006"
version               u32    current: 1
script_count          u32
text_blob_size        u32
reserved              u32
repeated script entries, 40 bytes each:
  route_offset        u32    offset into text blob
  route_size          u32
  source_offset       u32    offset into text blob
  source_size         u32
  code_offset         u32    offset into code blob
  code_size           u32
  order               u32    original document-order index
  flags               u32    inline/module/defer/async/external/remote metadata
  kind                u32    1 inline, 2 external
```

## v0.19 binary DOM-op flag

WASM builds set the `binary-dom-ops` package flag when `runtime.wasm` exposes the compact `VDOP0020` DOM operation stream through `venom_wasm_domops_ptr/size/capacity`. The JSON execution report remains diagnostics; the bridge decodes the binary stream as the executable DOM operation output.


## v0.19 async host-call bridge sections

v0.19 adds package flags and section types for the next host-call layer:

```txt
15 timer_bridge     timer-bridge.vtmr
16 event_queue      event-queue.vevq
17 quickjs_bridge   quickjs-bridge.vqjs
```

`async-host-queue.vahq` is now metadata version 3 and advertises fetch, timer, event, WASM, script isolation, and QuickJS chunk request boundaries. v0.19 also adds:

```txt
18 script_isolation  script-isolation.vsis
19 script_policy     script-policy.vscp
20 quickjs_chunks    quickjs-chunks.vqbc
```

QuickJS browser execution is a route-scoped chunk boundary, not a full QuickJS/WASM execution claim yet.


## v0.37 QuickJS engine adapter lifecycle sections

v0.37 keeps the QuickJS engine bootstrap flags, then adds adapter-lifecycle metadata for the browser module boundary:

```txt
quickjs-engine package flag
script-engine-fallback package flag
quickjs-engine-module package flag
quickjs-context-lifecycle package flag
host-capabilities package flag
quickjs-adapter-diagnostics package flag

quickjs_engine section type: quickjs-engine.vqse
script_engine_policy section type: script-engine-policy.vsep
quickjs_engine_module section type: quickjs-engine-module.vqem
quickjs_context_lifecycle section type: quickjs-context-lifecycle.vqcl
host_capabilities section type: host-capabilities.vhcap
quickjs_adapter_diagnostics section type: quickjs-adapter-diagnostics.vqad
```

`quickjs-engine.vqse` records the route-scoped reusable context model and host capability binding metadata. `quickjs-context-lifecycle.vqcl` records context create/reuse/destroy host calls. `host-capabilities.vhcap` records the injected browser bridge capabilities made visible to the engine adapter. `quickjs-adapter-diagnostics.vqad` records the adapter contract and fallback status so the future QuickJS/WASM engine can replace the current module behind the same interface.


## v0.37 QuickJS execution result bridge sections

v0.37 adds three metadata sections that make script execution auditable without claiming final QuickJS bytecode execution yet:

- `quickjs-execution-records.vqer` describes per-route execution records and runtime snapshot retention.
- `quickjs-result-bridge.vqrb` describes the JSON result record boundary emitted by the QuickJS/WASM scaffold and consumed by the JS adapter.
- `quickjs-fallback-policy.vqfp` describes the explicit policy gate for host-JS compatibility fallback.

The corresponding package flags are `quickjs-execution-records`, `quickjs-result-bridge`, and `quickjs-fallback-policy`.


## v0.37 QuickJS real engine ABI expansion sections

v0.37 also adds the explicit QuickJS runtime ABI expansion layer:

```txt
36 quickjs_runtime_abi       quickjs-runtime-abi.vqra
37 quickjs_host_imports      quickjs-host-imports.vqhi
38 quickjs_heap_limits       quickjs-heap-limits.vqhl
39 quickjs_script_buffer     quickjs-script-buffer.vqsb
40 quickjs_console_abi       quickjs-console-abi.vqca
41 quickjs_parity_probe      quickjs-parity-probe.vqpp
42 quickjs_release_fallback  quickjs-release-fallback.vqrf
43 quickjs_bytecode_manifest quickjs-bytecode-manifest.vqbm
44 quickjs_module_resolver   quickjs-module-resolver.vqmr
45 quickjs_exception_abi     quickjs-exception-abi.vqex
46 quickjs_host_trap_policy  quickjs-host-trap-policy.vqtp
```

These sections describe the runtime ABI table, host-call import design, context heap/stack accounting limits, script byte-buffer allocation API, console callback ABI, native/WASM parity probe, strict release fallback policy, bytecode buffer manifest records, package-relative module resolver contracts, exception record ABI, and host-trap policy. The JavaScript runtime parses and exposes them through `globalThis.__venomRuntime` so the real QuickJS backend can be swapped in behind the same verified boundary.


## v0.37 QuickJS backend replacement path

The package now includes `quickjs-engine-backend.vqeb`, `quickjs-native-parity.vqnp`, and `quickjs-execution-mode.vqxm`. These sections record backend selection, native QuickJS parity scope, execution mode, source-transfer requirements, result-bridge requirements, and explicit fallback policy. The generated QuickJS WASM asset exposes backend-status exports so the adapter can record whether the backend candidate accepted the chunk and whether fallback was required. Full in-browser QuickJS C bytecode execution is still not claimed.


## v0.37 release build policy

v0.37 adds `quickjs-release-build-policy.vqbp` as a release-policy record. Default release builds with packaged script chunks fail at build time while the browser QuickJS backend is still the scaffold candidate, unless the user passes the explicit unsafe fallback override. The policy record captures backend selection, script count, fallback allow/deny state, and the resulting build decision.

## v0.45 protected layout polymorphism

Protected/release packages keep the v40 outer container format, but v0.45 adds a per-build layout polymorphism layer:

- protected builds shuffle section table order using the active VM polymorphism seed;
- the writer adds randomized payload-layout jitter/padding between section payload offsets;
- padding bytes are randomized and are covered by the package hash;
- an encrypted `package-layout.vlay` metadata section records the layout policy, seed, section-order digest, and jitter limit;
- `venom release-check` requires the decoded layout metadata and reports `layout_polymorphic: yes` plus payload padding bytes.

The layout metadata is protected like other internal integrity sections, so raw `app.vbc` bytes must not expose `package-layout.vlay`, `VENOM_PACKAGE_LAYOUT_V1`, or the layout seed.

## v0.46 route-scoped lazy sections

Protected/release packages keep the v40 outer container format, but v0.46 adds encrypted `lazy-sections.vlazy` metadata. That metadata maps each route to a protected route-local VM bytecode section and, when needed, a protected route-local script bundle section.

The browser runtime now parses the package table without opening every protected payload. Section payloads are decoded through a lazy accessor, integrity-checked when first used, and cached for the current route/session. The legacy monolithic `route-bytecode.vmbc` and `scripts.vjsb` remain present for compatibility and native tooling, but browser execution prefers the route-scoped sections when `VENOM_LAZY_SECTIONS_V1` is available.

### Build-specific DOM command protocol

Protected Route VM execution emits the `VDOP0020` binary command stream. The stream carries a per-build physical-to-logical command map and a per-build permutation for the three payload fields in each command record. Both are derived from independent diversification domains in the protected build plan. The JavaScript DOM boundary validates the map, field permutation, record lengths, exact stream consumption, and supported logical operation set before applying mutations.

This protocol is self-describing for execution compatibility, but its physical identifiers and record layout differ between protected builds, preventing a single fixed-opcode extractor from working unchanged across distributions.

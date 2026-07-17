# Streamed WASM-owned package residency

> **Audience:** application integrators, security reviewers, and runtime contributors  
> **Applies to:** Venom 1.1.0
Venom 1.58 changes package ingestion so the browser loader no longer copies the complete package into WebAssembly memory in one operation or retains the fetched package bytes for later section decoding.

## Lifecycle

```text
fetch opaque package bytes
→ begin a bounded WASM upload session
→ copy 64 KiB chunks into WASM-owned package storage
→ validate monotonic chunk offsets
→ parse and bind the complete resident package inside WASM
→ clear the fetched JavaScript byte buffer
→ lazily materialize individual sections
→ copy only the requested output
→ explicitly zero the materialization buffer
```

The runtime rejects missing, reordered, overlapping, empty, or out-of-range chunks. A failed upload clears the resident package and metadata before returning an error.

## Short-lived plaintext

Encrypted or compressed sections are opened inside the package runtime. Materialized section bytes remain available only for the duration of the caller's copy and are then erased through the opaque runtime ABI. Route execution also clears transient route, string, bytecode, JavaScript, asset, result, and DOM-operation buffers after use.

## What this improves

- Removes the persistent `pkg.bytes` JavaScript package copy.
- Avoids repeatedly copying the full package before every section decode or route execution.
- Narrows obvious full-package instrumentation points.
- Adds a stateful, ordered package-upload contract.
- Ensures failed and retired resident packages are explicitly zeroed.
- Keeps section authentication, decompression, and materialization under the WASM runtime boundary.

## Security boundary

WebAssembly linear memory is still observable to a browser owner through developer tooling or engine-level instrumentation. Chunking and zeroization reduce exposure time and make generic hooks less convenient; they do not make client-delivered code permanently secret.

The strongest protection for true secrets remains a server-held trust boundary. Venom is designed to raise extraction and modification cost for client-side logic while preserving static deployment.

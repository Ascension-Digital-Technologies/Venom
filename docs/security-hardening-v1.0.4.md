# Venom v1.0.4 security hardening

## Security boundary

Protected JavaScript is compiled to a native QuickJS object with `JS_WriteObject`. The record contains a fixed `VQJSBC03` header followed by the native object bytes. The compiler requests both source and debug stripping. The browser runtime validates the record and passes the object bytes directly to `JS_ReadObject`, followed by `JS_EvalFunction`.

The protected runtime never reconstructs the original JavaScript source from a bytecode record. Legacy `VQJSBC01` and `VQJSBC02` records are rejected.

## Fail-closed WASM compatibility

A protected build is allowed only when the embedded QuickJS WASM provenance includes:

```text
bytecode_format=VQJSBC03
native_object_reader=JS_ReadObject
source_materialization=false
```

This prevents a compiler that emits V3 records from silently shipping an older runtime that expects the reversible V2 format. `tools/quickjs_wasm_cutover.py` also inspects the generated WASM and rejects artifacts that still contain the V2 marker.

## Package materialization

The companion package WASM no longer exports an index-only section decoder. Its materialization API requires the caller to supply the authenticated section type and raw size, and rejects a mismatch before opening or decompressing the section.

This narrows the API and prevents accidental type confusion. It does not make runtime data inaccessible to the owner of the browser.

## Release enforcement

The production builder sets the actual package release flag. Release checking scans all route-scoped JavaScript sections instead of depending on the removed monolithic `scripts.vjsb` section. It rejects legacy bytecode markers, obvious clear-source payloads, missing native bytecode records, source execution in a protected engine module, and stale QuickJS WASM provenance.

## Remaining limits

Native QuickJS bytecode is materially harder to recover than a reversible source envelope, but it is not an encryption boundary. Required string literals, property names, control flow, and behavior can still be analyzed from a client-owned artifact. Runtime package data can also be observed after materialization. Secrets and security-critical business logic must remain on a trusted server.

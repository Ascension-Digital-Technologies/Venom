# Large protected script transfer boundary

Venom v1.0.3 raises the QuickJS/WASM protected script transfer limit from 256 KiB to 768 KiB.

## Failure addressed

A protected `VQJSBC03` record larger than 262,144 bytes could not be accepted by `venom_qjs_script_buffer_alloc`, causing the browser runtime to stop with:

```text
Venom boot failed: QuickJS script byte buffer allocation failed
```

The failure is deterministic and occurs before script execution.

## Runtime design

The QuickJS runtime owns a dedicated 768 KiB transfer region. Protected records are copied into that region and validated inside WASM. The payload remains native QuickJS object bytecode in the transfer region. WASM validates the fixed header and payload hash, then passes the payload range directly to `JS_ReadObject`; no source-sized decode buffer or UTF-8 source boundary is created.

Large records continue through `venom_qjs_execute_bytecode`. There is no size-triggered host-JavaScript source execution path.

## Build-time enforcement

The compiler rejects source text or generated protected bytecode above 786,432 bytes before creating the output directory. This keeps production builds fail-closed and prevents incomplete distributions.

The browser runtime reports the requested byte count, runtime capacity, and WASM status metadata if allocation or bytecode validation fails.

## Regression coverage

The production loader test now includes a 540 KiB local protected script, exceeding the old 256 KiB boundary. A separate 790 KiB input is required to fail before `dist` creation.

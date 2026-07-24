# Build-specific bytecode envelopes

> **Audience:** Security reviewers, integrators, and runtime contributors  
> **Applies to:** Venom 1.1.0
Venom production packages no longer store protected TurboJS records directly as recognizable `VTJSBC03` or `VTJSMB04` byte sequences inside the JavaScript bundle. Each protected record is wrapped in a build-specific `VTJSE006` envelope with per-build lane permutation before package emission.

## Envelope lifecycle

```text
TurboJS compiler emits canonical record
→ compiler derives a build/route/source/order stream seed
→ canonical record is transformed into VTJSE006 storage form
→ envelope stores ABI fingerprint, payload length, and inner-record hash
→ package sealing and section diversification run normally
→ runtime materializes the protected section
→ runtime validates envelope ranges and ABI
→ runtime reverses the build-specific transform
→ runtime validates the inner-record hash and TurboJS magic
→ canonical record enters the TurboJS/WASM boundary
→ encoded and transient plaintext buffers are erased at their earliest safe points
```

## Build binding

The storage transform is derived from:

- the production build diversification salt;
- route identity;
- protected source identity;
- script execution order.

As a result, the same protected source compiled under different production builds does not produce the same stored byte sequence. Fixed signatures and one-build extraction scripts are therefore less reusable.

## Validation

The runtime rejects envelopes with:

- an unknown envelope magic or version;
- an incompatible TurboJS bytecode ABI fingerprint;
- invalid payload offsets or lengths;
- an inner-record integrity mismatch;
- an unsupported decoded record type.

Both native TurboJS object records (`VTJSBC03`) and protected module bundles (`VTJSMB04`) are accepted only after envelope validation.

## Security boundary

The envelope is a diversification and integrity mechanism, not a server-held encryption secret. An analyst controlling the browser can still instrument the runtime immediately before or after decoding. The benefit is that raw canonical TurboJS records are no longer directly present in package script sections, decoding is build-specific, malformed or stale records fail closed, and reusable static tooling becomes more expensive.

Truly secret keys, authoritative decisions, and irreplaceable proprietary data must remain on a trusted server.

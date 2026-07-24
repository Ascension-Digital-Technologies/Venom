# Binary protected values

Venom 1.1 supports `ArrayBuffer` and common numeric typed arrays as protected-call inputs and results. Binary bytes travel as raw attachments inside the authenticated bridge frame rather than being expanded into public JSON or Base64.

```javascript
const input = new Float32Array([1.5, 2.5, 3.5]);
const result = await venom.exports.transform(input);
console.log(result instanceof Float32Array);
```

Supported values are `ArrayBuffer`, `Uint8Array`, `Uint8ClampedArray`, `Int8Array`, `Uint16Array`, `Int16Array`, `Uint32Array`, `Int32Array`, `Float32Array`, and `Float64Array`. Values may be nested inside ordinary arrays and plain objects. `DataView`, `SharedArrayBuffer`, and BigInt typed arrays are rejected.

The public bridge retains the 1 MiB payload limit, a maximum of 64 binary objects per value, structural depth and node limits, per-call capability leases, replay protection, cancellation, and timeout enforcement. Input buffers are copied into the bridge frame; Venom does not detach caller-owned buffers.

## Typed contracts

Binary values can be declared in protected input/output contracts using lowercase type names such as `arraybuffer`, `uint8array`, `uint16array`, and `float64array`. Venom generates the corresponding TypeScript declarations and validates the reconstructed binary class inside the protected TurboJS boundary.

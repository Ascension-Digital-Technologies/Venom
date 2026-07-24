# Typed bridge contracts

Typed bridge contracts give protected exports an explicit, machine-readable boundary without requiring TypeScript as an input language.

## Declare a contract

Place `@venom: input` and `@venom: output` annotations directly before a protected function or a named function export in a protected module:

```javascript
// @venom: input symbol:string, quantity:integer, limit?:number
// @venom: output accepted:boolean, score:number
// @venom: protected
export async function assessOrder(input) {
  return {
    accepted: input.quantity > 0,
    score: input.quantity * (input.limit || 1)
  };
}
```

The contract describes the first argument and the returned object. Additional call options such as timeout and cancellation remain part of the browser API rather than the protected payload.

## Runtime enforcement

Venom emits validators inside the protected TurboJS bridge registry. Validation therefore occurs at the protected boundary even when browser-side code bypasses generated TypeScript declarations.

- Required fields must be present.
- Optional fields are checked when supplied.
- Numbers must be finite.
- Integer fields use integer semantics.
- Object fields reject arrays and null.
- Array contracts validate every element. Binary contracts validate the reconstructed `ArrayBuffer` or typed-array class.
- Promise results are validated after resolution.

## Generated integration files

Every build with typed protected exports creates private integration artifacts next to the distribution:

```text
.venom/<dist-name>/api/protected-contracts.json
.venom/<dist-name>/api/venom-exports.d.ts
```

`protected-contracts.json` is the canonical machine-readable contract document. `venom-exports.d.ts` supplies editor and TypeScript support for browser callers. Development builds also copy the declaration file into `assets/app/` for local tooling; hardened production output does not ship either document.

## Supported types

| Contract type | TypeScript representation | Runtime rule |
|---|---|---|
| `string` | `string` | JavaScript string |
| `number` | `number` | finite number |
| `integer` | `number` | `Number.isInteger` |
| `boolean` | `boolean` | Boolean value |
| `object` | `Record<string, unknown>` | non-null, non-array object |
| `array` | `unknown[]` | array |
| `any` | `unknown` | any JSON-safe value |
| `T[]` | corresponding array type | every element satisfies `T` |
| `arraybuffer` | `ArrayBuffer` | `ArrayBuffer` instance |
| `uint8array` | `Uint8Array` | `Uint8Array` instance |
| `uint8clampedarray` | `Uint8ClampedArray` | `Uint8ClampedArray` instance |
| `int8array` | `Int8Array` | `Int8Array` instance |
| `uint16array` | `Uint16Array` | `Uint16Array` instance |
| `int16array` | `Int16Array` | `Int16Array` instance |
| `uint32array` | `Uint32Array` | `Uint32Array` instance |
| `int32array` | `Int32Array` | `Int32Array` instance |
| `float32array` | `Float32Array` | `Float32Array` instance |
| `float64array` | `Float64Array` | `Float64Array` instance |

Malformed declarations fail closed with `VENOM-E2401`, unsupported types with `VENOM-E2402`, and duplicate fields with `VENOM-E2403`.

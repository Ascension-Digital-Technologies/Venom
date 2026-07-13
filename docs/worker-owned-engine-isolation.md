# Worker-owned engine isolation

Protected distributions launch a dedicated module worker before runtime boot. The worker exclusively performs package and QuickJS WASM network acquisition, applies hard byte ceilings, rejects redirects, compiles the QuickJS WebAssembly module off the main thread, and transfers the package buffer exactly once.

The main thread remains the bounded DOM capability endpoint. It receives no readable application source and cannot replace the worker path in protected profiles: missing Worker, protocol timeout, malformed replies, oversized resources, WASM compile failure, or worker startup failure aborts boot.

## Protocol

The `prepare` message is nonce-bound and limited to 1 KiB of serialized control data. Responses must echo the nonce and are either `ready` or `error`. The package buffer is transferred, not copied. The compiled `WebAssembly.Module` is structured-cloned and reused by the QuickJS engine module, avoiding a second fetch and compile.

## Limits

- Package: 64 MiB
- QuickJS WASM: 32 MiB
- Control message: 1 KiB
- Startup deadline: 15 seconds
- Redirects: denied
- Credentials: same-origin
- Cache mode: no-store

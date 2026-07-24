# Build profile

Venom 2.0.0 uses one supported build profile: `prod`. It always enables the real TurboJS/WASM runtime, fail-closed execution, encrypted package sections, integrity binding, polymorphic packaging, asset hashing, and stripped production metadata.

The former `dev` profile was removed because it maintained a separate runtime and packaging path that could diverge from production behavior. `venom dev` remains the local watch-and-serve command, but it builds through `prod`.

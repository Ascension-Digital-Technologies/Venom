# Phase 3B: Host Capability V3 Runtime Parser Fix

The compiler emitted `VENOM_HOST_CAPABILITIES_V3` while the embedded browser runtime accepted only V2 and V1, causing boot to fail with `invalid metadata header`.

The runtime now accepts V3, V2, and V1 and parses V3 policy, undeclared-capability behavior, default capabilities, and per-chunk capability records.

## Compatibility

- V3 least-privilege manifests: accepted
- V2 manifests: accepted
- V1 manifests: accepted
- Unknown headers: rejected

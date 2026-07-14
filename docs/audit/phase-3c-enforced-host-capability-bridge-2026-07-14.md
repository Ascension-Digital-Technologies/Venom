# Phase 3C — Enforced Host Capability Bridge

Development milestone: enforced host-capability bridge
Date: 2026-07-14

## Summary

The browser runtime now applies `VENOM_HOST_CAPABILITIES_V3` at QuickJS binding creation. Each script chunk receives only its declared host globals. The injected `__venomRuntime` value is a frozen capability-scoped facade rather than the complete bridge object.

## Enforcement

- Capability records are selected by chunk order, route, and source, with order-only fallback for opaque release labels.
- Undeclared `document`, `window`, `navigator`, `fetch`, timer, console, and event globals are not injected.
- Runtime methods for fetch, timers, events, and DOM operations are exposed only when the corresponding capability is declared.
- Explicit capability checks fail with `VNM-CAP-1001`.
- V1/V2 compatibility remains permissive according to their metadata because they do not provide per-chunk records.

## Residual risk

This boundary constrains host objects injected by Venom. It does not turn an untrusted browser into a trusted environment and does not prevent observation of values legitimately returned through declared capabilities.

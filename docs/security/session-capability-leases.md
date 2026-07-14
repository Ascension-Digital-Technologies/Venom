# Session capability leases

> **Applies to:** Venom 1.65.2
Venom 1.63 rotates the protected bridge protocol for every worker session and replaces stable transport capabilities with request-scoped capability leases.

## Lifecycle

```text
worker session starts
→ random generation and session key are created
→ invoke/cancel/result/error opcodes are rotated
→ each request derives a lease from capability + generation + key + counter
→ worker reconstructs the expected lease for that counter
→ monotonic counter consumes the lease
→ replay or cross-session reuse fails closed
```

The public API remains `venom.exports.*`; names and stable capability values are not transmitted.

## Security benefit

A captured invocation frame cannot be reused for another request or worker generation. Fixed protocol-opcode hooks and stable-capability matchers also become less reusable between sessions.

This is still a browser-controlled environment. An analyst able to instrument both endpoints can observe or reproduce session derivation. The feature raises automation and replay cost; it is not a server-held authorization secret.

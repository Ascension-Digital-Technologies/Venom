# Runtime resource policy

Venom protected browser builds enforce route-scoped resource budgets in the generated runtime. These limits are designed to contain accidental runaway behavior and make denial-of-service through the host bridge more difficult.

The canonical defaults are stored in `contracts/runtime-resource-policy.json`.

## Enforced budgets

- 128 pending host calls per route
- 128 retained completed host-call records
- 8,192 total host calls per route generation
- 16 concurrent fetches
- 64 KiB maximum fetch request metadata/body bridge size
- 1 MiB maximum fetch response body
- 128 active timers
- 512 timer schedules per route generation
- 256 retained event records
- 1,024 event dispatches per route generation
- 4,096 DOM handles
- 24-hour maximum route-generation lifetime between host operations

## Fail-closed behavior

Budget exhaustion throws a stable runtime error and does not fall back to unrestricted browser execution. Route changes cancel timers, abort fetches, reject pending operations, reset counters, destroy route-owned QuickJS contexts, and invalidate DOM handles.

These limits are browser-runtime containment controls. They do not replace server-side rate limiting, request authentication, response-size controls, or application-level authorization.

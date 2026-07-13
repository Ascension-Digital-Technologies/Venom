# Application-specialized browser bridge

Venom v1.19.0 emits `VENOM_HOST_BRIDGE_V14`, a deny-by-default, application-specialized numeric capability manifest.

The compiler scans the protected JavaScript chunks and emits only the asynchronous browser capabilities the application actually uses. Core route/DOM capabilities remain present, while fetch, timer, and event queue records are omitted when their corresponding surfaces are absent. Unknown capability and host-call identifiers remain denied.

Each capability uses a fixed request/response schema and per-call byte limits. Protected runtime code does not enable generic global lookup, generic property traversal, generic method invocation, source-string transfer, or function-object transfer across the bridge.

DOM references use generation-scoped, authenticated DOM handles. A per-session secret from `crypto.getRandomValues()` authenticates the packed handle value; malformed handles, stale route generations, unknown slots, and nodes outside the owned route root are rejected.

This specialization reduces exposed semantic metadata and unused browser functionality. It is a least-privilege and attack-surface reduction mechanism, not a secrecy boundary against the browser operator.

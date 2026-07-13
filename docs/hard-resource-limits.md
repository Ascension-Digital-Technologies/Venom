# v0.93.0 hard resource limits and runtime isolation

Protected runtime queues are deny-by-default and bounded. A route may hold at most 256 pending and 256 retained completed operations. Fetch request metadata is capped at 64 KiB and response bodies at 1 MiB. Timers are capped and clamped to a 24-hour maximum delay. Queue disposal cancels timers, aborts active fetches, rejects pending operations, and prevents further enqueue operations.

QuickJS contexts retain the existing WASM-enforced 8 MiB heap, 512 KiB stack, 768 KiB script-buffer, 64-context, 4096 host-call, 1024 console-event, and 512 module-record ceilings. Protected execution remains fail-closed when any limit is exceeded.

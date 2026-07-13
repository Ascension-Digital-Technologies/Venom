# Production site test fixture

This mirrors `examples/protected-chess` for offline regression tests. The Cloudflare analytics beacon is omitted because the deterministic no-op remote cache fixture cannot satisfy the production site's official SHA-512 SRI value. SRI verification is covered separately by `venom_remote_vendor_smoke` for SHA-256, SHA-384, and SHA-512.

# Runtime Management

> **Applies to:** Venom 1.0.1

A Venom distribution binds its loader, package, workers, JavaScript runtime assets, and WebAssembly binaries as one verified release unit. Do not mix runtime files from different builds.

## Deployment rule

Deploy the complete generated `dist/` atomically. If a CDN or object store uploads files independently, publish content-addressed assets first and update `index.html` only after all referenced assets are available.

## Cache policy

Hashed assets may use long-lived immutable caching. Stable entry documents should use shorter cache lifetimes or revalidation so they can point clients to the new asset set after deployment.

## Runtime verification

Before deployment:

```powershell
venom verify-runtime dist
venom release-check dist
```

These checks validate runtime identity, package binding, asset hashes, expected exports, fail-closed policy, and production metadata.

## Rollout

Use staged rollout for high-traffic applications. Preserve the previous complete distribution until the new version passes health checks. Roll back by restoring the previous complete asset set and entry document, not by copying individual runtime files.

## Diagnostics

A runtime mismatch should be treated as an incomplete or corrupted deployment. Rebuild or redeploy the entire distribution. Never disable integrity verification to accommodate a partially updated cache.

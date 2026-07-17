# Routing

> **Applies to:** Venom 1.1.0

Venom supports static multi-route sites and browser-side navigation while packaging route structure into the production distribution. Route hydration reconstructs the required DOM and schedules browser/protected scripts in the expected order.

## Route discovery

Routes are discovered from the site graph and source files. Keep route entry files inside the configured site root and use web-relative links. Nested routes must resolve assets relative to the distribution asset base rather than the source filesystem.

## Navigation behavior

Browser-native navigation, history operations, and framework routers remain browser-side. Protected logic may calculate route decisions, permissions, or data, but the actual DOM and history integration should use browser-executed code.

## Production route runtime

The Route VM uses build-specific physical opcodes and instruction layouts. The logical route behavior stays stable while the physical representation may vary across production builds.

## Verification

Test direct navigation, refresh, back/forward navigation, nested routes, route-specific assets, and deployment under the intended base path. Use browser equivalence for observable source-versus-protected behavior:

```powershell
.\scripts\windows\release-closure.ps1 
```

See [Route hydration](../architecture/route-hydration.md) and [Deployment](../getting-started/deployment.md).

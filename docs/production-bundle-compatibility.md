# Production-bundle compatibility

Venom validates compatibility with generated distributions through fixture-specific behavioral contracts. Version 1.8.0 adds a production-style ES module fixture that resembles the output shape of a modern static bundler without claiming compatibility with a specific framework.

## Covered bundle characteristics

The fixture exercises:

- content-hashed JavaScript and CSS filenames;
- compact/minified ES module syntax;
- a multi-file static import graph;
- shared utility and feature chunks;
- DOM hydration-style updates;
- event listeners, promises, and timers;
- multiple HTML routes;
- generated CSS delivery.

The browser report records the fixture profile, evidence level, claims, manifest digest, and distribution digest. The compatibility matrix preserves those fields for each browser result.

## Claim boundary

Passing this fixture supports claims about the bundle characteristics above. It does not establish general support for React, Vue, Svelte, Angular, Next.js, Nuxt, or another framework. A framework is listed as supported only after an actual production build from that framework is checked into the fixture corpus and passes the real-browser matrix continuously.

## Future framework fixtures

Framework fixtures should be generated from pinned toolchain and dependency versions, retain their lockfiles and build instructions, and test real interaction behavior rather than boot alone. Failed or experimental fixtures should remain clearly labeled and must not be used as support claims.

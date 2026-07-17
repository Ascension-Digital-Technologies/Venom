# Runtime startup observability

Venom 2.0.0 publishes an authoritative startup timeline through `globalThis.__venomBootStatus` and the `venom:boot-phase` event. The timeline measures package loading, policy verification, runtime installation, lazy route decoding, DOM rendering, script execution, and navigation setup.

```js
addEventListener('venom:boot-phase', (event) => {
  console.log(event.detail.phase, event.detail.durationMs);
});
```

A complete status contains the attempt number, total duration, immutable phase timeline, and structured detail. Startup failures use stable `VENOM_BOOT_*` codes. Recovery is deliberately explicit: `globalThis.__venomRetryBoot()` reloads the document so partially initialized workers, package state, and DOM mutations are not reused.

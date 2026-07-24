# Phase 1H: Bot telemetry clock-domain fix

## Root cause

The browser created `capturedAtMs` using the browser's live `Date.now()`. The protected TurboJS/WASM context independently evaluated `Date.now()` and could expose a non-advancing or differently based clock. The protected engine compared those clocks with a fixed 30-second tolerance, eventually rejecting fresh requests as `stale-telemetry`.

## Fix

Telemetry freshness is now enforced inside the authenticated assessment session using:

- session nonce binding;
- strictly ordered sequence numbers;
- monotonic `capturedAtMs` values per session;
- session expiration and active-session bounds.

The protected engine no longer assumes browser and TurboJS wall clocks are synchronized. The browser also renegotiates once on `stale-telemetry` as a defensive recovery path.

## Security effect

Replay and out-of-order messages remain rejected by the nonce and sequence gate. A capture timestamp that moves backwards within a session is rejected. This removes false stale failures without weakening the existing one-time ordered envelope contract.

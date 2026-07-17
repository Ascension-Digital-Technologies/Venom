# Batched protected calls

Venom 1.1 can schedule several independent protected export calls through one public API operation.

```javascript
const results = await venom.batch([
  { name: "scoreOrder", input: firstOrder },
  { name: "scoreOrder", input: secondOrder },
  { name: "checkLicense", input: license }
]);
```

Each item contains a protected export `name`, an `input` value, and optional per-call `options`. A shared options object may be passed as the second argument:

```javascript
await venom.batch(calls, { timeout: 10000 });
```

The current `parallel-v1` batch contract schedules validated calls concurrently through the existing leased binary capability transport. It does not combine unrelated calls into one capability lease, so each call retains its own timeout, cancellation, replay counter, and response validation.

The maximum batch size equals the public pending-call limit. An empty batch resolves to an empty array. Invalid items and unknown exports fail before any calls are scheduled.

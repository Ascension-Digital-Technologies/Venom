# Chrome extension protected RPC

Venom's Manifest V3 extension target emits a versioned RPC boundary when the
runtime host is an offscreen document.

## Generated files

- `venom-extension-rpc.js` — client API exposed as `VenomExtensionRPC.call()`.
- `venom-extension-broker.js` — service-worker broker and offscreen lifecycle recovery.
- `venom-extension-host.js` — protected export dispatcher hosted by `engine.html`.
- `venom-background.js` — generated service-worker wrapper that loads the
  developer worker and the Venom broker.

The source service worker remains unchanged. Venom patches the emitted manifest
to point at the wrapper. Module workers use static imports; classic workers use
`importScripts()`.

## Protocol limits

The `venom-extension-rpc-v1` protocol validates export names, JSON-serializable
payloads, payload size, timeout bounds, and maximum in-flight requests. The
host never accepts arbitrary source code or property paths—only a single named
protected export from `venom.exports`.

Default limits:

- 1 MiB serialized payload
- 32 concurrent host calls
- 20 second default timeout
- 60 second maximum timeout

## Lifecycle recovery

The broker creates the offscreen document when needed, waits for a versioned
`PING`, and retries once after a failed send. This handles Manifest V3 service
worker suspension and offscreen-document restart without exposing protected
logic to the service worker or content scripts.

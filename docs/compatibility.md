# Compatibility

Venom targets supported static and client-rendered websites whose JavaScript and browser API usage are represented by the current QuickJS host bridge.

## Supported areas

The repository contains support or infrastructure for:

- ordinary HTML and CSS collection;
- multiple HTML routes;
- inline and external scripts in document order;
- selected ES module workflows;
- common DOM lookup, construction, mutation, attributes, and events;
- timers, promises, console output, fetch, storage, navigation, and forms;
- selected browser-global, media, and feature-detection shims;
- remote HTTPS script vendoring and lockfile reuse.

The exact support level of an API depends on the generated runtime and host bridge implementation. Presence in source code or metadata does not automatically imply complete browser-semantic parity.

## Unsupported or intentionally restricted areas

Applications may require changes when they depend on:

- `eval`, `new Function`, or runtime source decoding;
- Node.js APIs such as `require`, `process`, or `fs`;
- browser extensions;
- undocumented browser internals;
- exact identity or prototype behavior of native DOM objects;
- unsupported Web APIs;
- framework behavior that relies on unimplemented host semantics;
- environments without WebAssembly, workers, typed arrays, promises, or fetch.

## Browser status

| Target | Status |
|---|---|
| Chromium-based desktop browsers | Primary development and validation target |
| Firefox | Intended target; release and application validation required |
| Safari/WebKit | Intended target; release and application validation required |
| Mobile Chromium/WebView | Application-specific testing required |
| Legacy browsers | Unsupported |

Venom should not publish exact minimum browser versions unless those versions are exercised by CI or a release qualification matrix.

## Compatibility preflight

```bash
venom compatibility check ./site
venom compatibility check ./site --format json
```

The scanner identifies known denied, unsupported, or partially supported patterns. It is advisory and cannot prove runtime correctness.

## Required application validation

For every production application:

1. build with the real pinned QuickJS/WASM artifact;
2. run `venom release-check` and `venom verify-runtime`;
3. serve the complete distribution over the intended production server configuration;
4. test direct navigation and refresh for every route;
5. exercise critical DOM, event, storage, network, and error paths;
6. test every claimed target browser and relevant device class;
7. compare important observable behavior with the unprotected application.

A successful build is not a compatibility certification.

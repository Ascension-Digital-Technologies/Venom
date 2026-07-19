# Chrome extension builds

Venom supports Chrome Manifest V3 projects through:

```toml
[project]
target = "chrome-extension"
```

The extension project must contain a root `manifest.json` and at least one HTML extension page, such as an action popup or options page. Venom builds the HTML page through the sealed QuickJS/WASM route pipeline, preserves stable packaged paths required by `chrome.scripting.executeScript`, and emits a load-unpacked directory.

The emitted Manifest V3 policy includes:

```text
script-src 'self' 'wasm-unsafe-eval'; object-src 'self'
```

This permits the embedded QuickJS WebAssembly runtime while retaining the Manifest V3 prohibition on remote code and inline executable scripts.

## Example

On Windows:

```bat
scripts\windows\build-and-launch-chrome-extension.bat
```

The launcher builds and verifies the extension, opens the output folder, and opens `chrome://extensions/`. Enable Developer mode and select **Load unpacked**.

## Current boundary

Extension page routes and annotated Venom exports use the sealed package. Files referenced by `chrome.scripting.executeScript` retain stable root-relative names because Chrome requires literal packaged paths. Treat those files as browser adapters and keep sensitive algorithms behind protected exports or extension-page calls.

## Manifest analysis and resource closure

Venom parses Manifest V3 structurally before compilation. The extension build records and validates resources referenced by:

- action popups and icons;
- background service workers;
- options pages and side panels;
- DevTools pages and Chrome URL overrides;
- isolated-world and main-world content scripts;
- sandbox pages;
- Declarative Net Request rule resources;
- web-accessible resources with concrete paths;
- HTML `src` and `href` attributes;
- CSS `url(...)` references;
- static JavaScript imports;
- `chrome.runtime.getURL(...)` calls;
- statically declared `chrome.scripting.executeScript` file arrays.

Only files reachable from these extension entrypoints are copied to the stable root layout. Development scripts, tests, documentation, and other unrelated project files are not traversed merely because they exist in the source directory.

A production build fails if a required manifest resource is missing. Diagnostics identify the missing path, the manifest field that referenced it, and its execution context, such as `service-worker`, `content-script-main`, or `extension-page`.

Dynamic resource names that cannot be determined statically must be declared explicitly by the project or added after the distribution is produced. Wildcard web-accessible-resource declarations are preserved in `manifest.json`, but Venom does not use a wildcard as permission to copy arbitrary source-tree files.

## Context-aware compilation

Venom assigns every executable extension resource to its actual Chrome execution world. JavaScript imports inherit the world of their manifest entrypoint rather than being treated as generic static files.

| Context | DOM | `chrome.*` APIs | Direct QuickJS/WASM host | Venom treatment |
|---|---:|---:|---:|---|
| Extension page | Yes | Yes | Yes | May host protected exports directly |
| Service worker | No | Yes | Worker-safe only | Stable adapter or worker runtime |
| Isolated content script | Yes | Yes | No | Visible DOM adapter using RPC |
| Main-world script | Yes | No privileged API | No | Minimal page bridge using RPC |
| DevTools page | Yes | Yes | Yes | Extension-page protection model |
| Sandbox page | Yes | No privileged API | No | Isolated visible adapter |

The runtime-host planner prefers an offscreen document when the manifest includes the `offscreen` permission and `minimum_chrome_version` is at least 109. It otherwise selects an existing extension page and finally a worker-safe service worker host. Builds fail when no compatible protected runtime host exists.

Protected source may not be referenced directly from a main-world content script or sandbox page. Those contexts must use a visible adapter and pass bounded data to a protected export.

Literal `chrome.scripting.executeScript({files:[...]})` entries are classified as isolated content scripts and retain stable package paths. Dynamic imports or script arrays that cannot be resolved statically are reported as compatibility warnings so they can be declared explicitly.


## Compatibility verification

Venom ships a 12-fixture Manifest V3 corpus and a five-context real Chrome
lifecycle suite. On Windows, run:

```bat
scripts\windows\verify-chrome-extension-compatibility.bat
```

The native matrix validates manifest analysis, resource reachability, execution
context inheritance, runtime-host selection, localization, wildcard
web-accessible resources, and static content-script registration. The browser
matrix loads fixtures through Chrome's **Load unpacked** path and checks popup,
service-worker, isolated-world, main-world, and offscreen behavior.

See `docs/reference/chrome-extension-compatibility.md` for the published support
matrix and boundaries.

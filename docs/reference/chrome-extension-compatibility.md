# Chrome Extension Compatibility Matrix

Venom targets standards-compliant **Manifest V3** extensions. Compatibility is
verified through a native 12-fixture corpus, with real Chromium lifecycle tests
for the five execution contexts most likely to break during packaging.

## Support tiers

- **Tier A — fully supported:** compiled, emitted, and covered by the native compatibility corpus.
- **Tier B — adapter-aware:** supported when privileged or protected work is routed through the generated RPC boundary.
- **Tier C — validation only:** recognized, but not automatically transformed into protected execution.

| Feature | Tier | Native fixture | Chromium fixture |
|---|---|---:|---:|
| Action popup and extension pages | A | Yes | Yes |
| ES-module service workers | A | Yes | Yes |
| Isolated-world content scripts | A | Yes | Yes |
| Main-world content scripts | B | Yes | Yes |
| Offscreen protected runtime host | A | Yes | Yes |
| Options pages and side panels | A | Yes | Planned |
| DevTools and sandbox pages | B | Yes | Planned |
| Declarative Net Request rules | A | Yes | Planned |
| Web-accessible resources, including wildcards | A | Yes | Planned |
| Localization and icons | A | Yes | Planned |
| Statically declared dynamic content registration | B | Yes | Planned |
| Multiple HTML routes and URL overrides | A | Yes | Planned |

## Important boundaries

Service workers may host the worker-safe runtime without an HTML route. Popups,
options pages, side panels, and DevTools pages can host a DOM-capable extension
runtime. Main-world scripts and sandbox pages remain visible adapters and call
protected exports through the generated, bounded RPC layer.

Venom preserves files reachable from the manifest and dependency graph. It also
expands wildcard `web_accessible_resources`, retains all packaged `_locales`
resources when `default_locale` is present, and discovers literal files passed
to `executeScript` or `registerContentScripts`.

## Running the matrix

Native matrix:

```bash
cmake --build build --target venom_chrome_extension_compatibility_matrix_test
ctest --test-dir build -R chrome_extension_compatibility_matrix_smoke --output-on-failure
```

Real Chromium lifecycle matrix:

```bash
python tests/browser/chrome-extension-compatibility.py
```

Set `VENOM_CHROMIUM` when Chrome or Chromium is not discoverable automatically.
The browser suite loads each fixture unpacked in an isolated profile and checks
popup startup, module-worker messaging, isolated/main-world injection, and
offscreen document reuse.

## Canonical distribution layout

Chrome-extension builds use the same Venom production asset tree as website builds. Only `manifest.json` remains at the distribution root because Chrome requires it there. Manifest-facing extension files are grouped beneath `extension/` while sealed runtime artifacts remain in the standard `assets/` directories.

```text
manifest.json
extension/
  popup.html
  popup.js
  background.js
  content-script.js
  engine.html
  venom-background.js
  venom-extension-rpc.js
  venom-extension-broker.js
  venom-extension-host.js
assets/
  app/
  javascript/
  wasm/
```

Venom rewrites manifest references and statically discoverable Chrome API resource literals to their canonical `extension/` paths. Authored relative HTML, CSS, and module imports remain relative because the complete extension shell moves as one directory.

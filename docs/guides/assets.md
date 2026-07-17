# Assets

> **Applies to:** Venom 1.0.1

Venom copies and rewrites site assets into the generated static distribution while preserving route-relative behavior. Production assets are content addressed where required and are bound to the generated loader and package metadata.

## Supported asset categories

- images and icons;
- CSS and CSS-referenced resources;
- fonts;
- browser-executed JavaScript chunks;
- JSON and other static data files;
- worker assets;
- route-specific resources.

Images are placed beneath `assets/images/`. Runtime, loader, style, worker, and package files use their dedicated production directories described in [Output layout](../reference/output-layout.md).

## Source references

Use normal relative or root-relative web paths in HTML and CSS. Venom resolves them against the source route, copies the target, and rewrites the generated reference. Avoid filesystem-only paths, drive letters, and resources outside the configured site root.

## Remote assets

Remote dependencies should be explicitly qualified. When remote vendoring is enabled, Venom records the resolved URL and integrity information so production builds do not silently depend on mutable third-party content.

## Production checks

After building, run:

```powershell
venom analyze-dist dist
venom release-check dist
```

Confirm that every expected image, font, and stylesheet is present, that no source-only files were copied, and that routes load correctly from the intended hosting base path.

## Troubleshooting

A missing asset usually indicates an incorrect source-relative path, unsupported dynamic path construction, a file outside the site root, or a base-path mismatch. Use browser network diagnostics together with [Debugging](debugging.md).

# Assets and remote vendoring

Local images are emitted under `assets/images/`. Styles, loaders, workers, runtime modules, packages, and WASM are separated by responsibility.

Remote scripts should be vendored for deterministic production builds. Venom records remote source locks and validates release assets. Redirects, MIME types, integrity data, and content hashes should be reviewed when a third-party CDN changes behavior.

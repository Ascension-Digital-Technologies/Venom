# Production Output Layout

> **Applies to:** Venom 3.5.0 hardened production builds

A production build emits a static distribution with stable entry documents and anonymous, content-addressed protected assets.

```text
dist/
├── index.html
└── assets/
    ├── app/
    │   ├── <hash>.vbc
    │   ├── <hash>.vqc        # optional lazy protected chunks
    │   └── <hash>.css
    ├── javascript/
    │   └── <hash>.js         # loader, runtime bridge, engine, and worker assets
    ├── wasm/
    │   └── <hash>.wasm
    └── images/
        └── <name>.<hash>.<ext>
```

## Responsibilities

- `index.html` starts the verified bootstrap asset.
- The `.vbc` file contains the protected application package and route data.
- Optional `.vqc` files contain authenticated lazy protected chunks.
- CSS is emitted under `assets/app/`.
- Loader, runtime bridge, TurboJS engine, and worker code are anonymous `.js` files under `assets/javascript/`.
- Runtime modules are anonymous `.wasm` files under `assets/wasm/`.
- Reachable image assets are emitted under `assets/images/` with content hashes.

The hardened layout deliberately does not expose role-based directories or filenames such as `assets/loader/`, `assets/runtime/`, `assets/style/`, `assets/workers/`, `loader.*`, `runtime.*`, or `worker.*`.

## Private build reports

Build provenance, protection intent records, and generated bridge contracts are written outside the public distribution to the sibling `.venom/<output-directory>/` report directory. They are not deployment assets.

## Excluded content

Production output must not contain source maps, original protected source, extraction reports, browser-test manifests, private keys, internal engineering documents, public build metadata, or files from another build.

Deploy the distribution directory atomically and verify it with `analyze`, `verify-runtime`, and `verify`.

## Private diagnostics layout

Internal reports are not public distribution assets. For an output directory named `dist-venom`, Venom writes private diagnostics to the sibling location:

```text
.venom/dist-venom/
├── build.json
├── build-performance.json
├── protection-intents.json
├── api/
└── reports/
```

Automation that consumes provenance, execution plans, bridge reports, generated API contracts, or performance evidence must read this private tree. Public deployment tooling should copy only the production output directory.

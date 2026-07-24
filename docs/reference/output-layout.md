# Production Output Layout

> **Applies to:** Venom 1.1.0

A production build emits a static distribution with stable entry documents and content-addressed protected assets.

```text
dist/
├── index.html
└── assets/
    ├── app/
    │   ├── app.<hash>.vbc
    │   └── build.json
    ├── images/
    ├── loader/
    │   └── loader.<hash>.js
    ├── runtime/
    │   ├── engine.<hash>.js
    │   ├── r.<hash>.js
    │   ├── runtime.<hash>.wasm
    │   └── rw.<hash>.wasm
    ├── style/
    │   └── s.<hash>.css
    └── workers/
        └── worker.<hash>.js
```

## Responsibilities

- `index.html` starts the verified loader.
- `app.<hash>.vbc` contains the protected package and route data.
- `loader.<hash>.js` resolves and validates the build assets.
- runtime JavaScript and WASM files implement package decoding, route hydration, and TurboJS execution.
- worker files host protected execution and the private binary bridge.
- `build.json` records non-secret build identity and binding metadata required by verification.

## Excluded content

Production output must not contain source maps, original protected source, extraction reports, browser-test manifests, private keys, internal engineering documents, or files from another build.

Deploy the directory atomically and verify it with `analyze`, `verify-runtime`, and `verify`.

# Production output layout

```text
dist/
├── index.html
└── assets/
    ├── app/       protected package and build metadata
    ├── images/    local image assets
    ├── loader/    verified bootstrap loader
    ├── runtime/   QuickJS engine, runtime JS, and WASM
    ├── style/     compiled stylesheets
    └── workers/   dedicated worker runtime
```

Hashed production assets should be cached immutably. `index.html` and release metadata should use shorter cache lifetimes.

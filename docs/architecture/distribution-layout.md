# Production distribution layout

Hardened production builds emit a minimal role-neutral asset tree:

```text
dist/
├── index.html
└── assets/
    ├── app/
    │   ├── <16-hex>.vbc
    │   ├── <16-hex>.vqc
    │   └── <16-hex>.css
    ├── javascript/
    │   └── <16-hex>.js
    ├── wasm/
    │   └── <16-hex>.wasm
    └── images/
        └── <normalized-name>.<16-hex>.<ext>
```

Production output contains no JSON manifest. Boot-critical asset references are embedded into the integrity-bound JavaScript/package data. Detailed build provenance is written outside `dist/` under the private `.venom/<dist-name>/` report directory.

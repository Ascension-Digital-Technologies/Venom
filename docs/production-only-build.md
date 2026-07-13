# v1.0.1 production-only app/style/runtime layout

Venom has one website build path: the hardened browser production target.

`venom build <site-dir> --out <dist>` always normalizes to:

- `profile=browser-protect`
- `runtime=wasm`
- `quickjs_backend=wasm-real`
- hashed assets enabled
- strict release enabled
- host JavaScript fallback denied
- external debug manifest disabled
- browser target metadata

The dist layout is:

```text
dist/
  index.html
  assets/
    app/
      app.<hash>.vbc
    style/
      s.<hash>.css
    loader/
      loader.<hash>.js
    runtime/
      engine.<hash>.js
      r.<hash>.js
      runtime.<hash>.wasm
      rw.<hash>.wasm
    workers/
      worker.<hash>.js
```

Historical debug/protect/compat flags remain parse-compatible for older scripts, but they no longer change website build output. This keeps the tool simple: every compiled site is the production artifact.

v1.0.1 also adds a production loader smoke gate. The test imports the generated `assets/loader/loader.<hash>.js`, requires the loader to resolve `assets/app/`, `assets/style/`, `assets/runtime/`, and `assets/workers/`, runs worker package/WASM preflight, delivers the package binding token, and boots with source eval/`Function` blocked.
## v1.0.1 remote dependency policy

Production builds vendor static HTTPS `<script src>` dependencies at compile time. The downloaded bytes are validated, optionally checked against SHA-256, SHA-384, or SHA-512 SRI metadata, compiled into the protected package, and cached for reproducible offline rebuilds.

```text
venom build <site> --out dist
venom build <site> --out dist --offline
venom build <site> --out dist --refresh-vendors
```

The release verifier rejects packages containing runtime remote-script URL chunks. Dynamic URLs created by application code remain application-controlled and are not treated as static build inputs.


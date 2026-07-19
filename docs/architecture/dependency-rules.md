# Source-domain dependency rules

Venom compiles every first-party source domain independently before aggregating the object files into `venom_core`. Directory ownership is therefore a build boundary rather than a visual convention.

## Dependency direction

```text
base
 ├── package
 │    ├── core
 │    ├── quickjs
 │    └── vm
 ├── frontends
 │    └── graph
 ├── remote
 ├── runtime
 └── pipeline
      └── cli
```


`runtime` is a native service boundary that depends on `core` and generated runtime assets. `generated` is a special implementation domain: generated C++ snapshots may include the pipeline or runtime declarations they implement, and consumers may include generated contracts and embedded blobs. It is excluded from authored-cycle analysis but still uses an explicit dependency allowlist.

The diagram is intentionally simplified; `pipeline` coordinates most lower-level domains, while `generated` is a special snapshot domain described below.

## Enforced rules

- `base/` must not include another Venom source domain.
- No reusable source domain may include `cli/`.
- Frontends do not include pipeline implementation headers.
- The canonical graph does not depend on the pipeline.
- Authored domains must remain acyclic.
- Every cross-domain include must appear in the checked allowlist.
- Every compiled domain has a dedicated API interface target and implementation object target.
- Product targets must not expose the repository-wide `src/` directory.
- Cross-domain includes use `venom/<domain>/...` and must resolve beneath the owner domain's `include/venom/<domain>/` tree.
- Public headers may not include private implementation headers.
- Private headers may only be included by their owning domain or an explicitly scoped internal test target.

Run the checks directly with:

```sh
python tools/architecture/check_domain_dependencies.py .
python tools/architecture/check_header_boundaries.py .
```

CTest registers the same checks as `venom_source_domain_dependencies_smoke` and `venom_header_boundaries_smoke`.

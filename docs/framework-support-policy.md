# Framework support policy

Venom separates technical experiments from public compatibility claims.

## Tiers

- `probe`: exact framework/version/delivery artifacts are compiled and measured, but no support claim is made.
- `candidate`: some required browser evidence exists, but the full matrix or behavioral depth is incomplete.
- `supported`: every required browser passes the fixture's bound behavioral contract and minimum check count.

The default required browser set is Chromium, Firefox, and WebKit. The policy is enforced by `tools/compatibility_matrix.py`; documentation cannot promote a fixture beyond the tier justified by its reports.

A support claim applies only to the exact framework version, delivery form, and behaviors recorded by the fixture. It does not automatically cover other versions, routers, SSR, hydration, plugins, or ecosystem packages.

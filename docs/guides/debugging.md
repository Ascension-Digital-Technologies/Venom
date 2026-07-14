# Debugging

1. Run `venom doctor --profile production`.
2. Reproduce with `venom dev <site> --open`.
3. Run `venom compatibility check <site>`.
4. Inspect browser and worker console output.
5. Verify remote assets and MIME types.
6. Build production and run `venom analyze-dist` plus `venom release-check`.

Development uses the real protected runtime but preserves readable generated JavaScript and richer diagnostics. Do not debug production by enabling host fallback; fix the compatibility boundary instead.

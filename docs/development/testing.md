# Testing

Run all tests:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Important groups include static contracts, compiler integration, runtime probes, route hydration, browser validation, package verification, security gates, reproducibility, and release closure. New behavior should include the narrowest useful unit/static test plus an executable integration regression.

# Aegis Operations

A large production-style Venom stress test. It combines a deep TypeScript/TSX browser graph with protected QuickJS/WASM analytics services.


## Screenshot

![Aegis Operations command center](../../docs/assets/examples/aegis-operations/application.png)

## Coverage

- 25+ TypeScript, TSX, and JavaScript modules
- Protected multi-module analytics pipeline
- Five asynchronous protected exports
- Type-only imports, default exports, barrels, and re-exports
- Stateful dashboard navigation, filters, search, modals, and live refresh
- KPI cards, risk distribution, trend visualization, incident table, asset inventory, activity feed, reports, and settings
- JSON and JSON-LD script data blocks
- Source leak verification and production package/runtime verification

## Run

From the repository root in PowerShell:

```powershell
.\scripts\windows\build-and-launch-aegis-operations.bat
```

Open `http://127.0.0.1:8085/`.

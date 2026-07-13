# CLI contract

Venom commands use stable process exit categories:

| Code | Category |
|---:|---|
| 0 | Success |
| 10 | Invalid command or configuration |
| 20 | Source compatibility failure |
| 30 | Package or verification failure |
| 40 | Runtime integrity or key operation failure |
| 50 | Toolchain/environment failure |
| 60 | Release-policy failure |
| 70 | Internal compiler/build failure |

Automation-oriented commands support `--format json`. Venom 1.1 currently provides structured output for `doctor` and `build`; verification and inspection commands retain their text output while their JSON contracts are finalized.

```bash
venom doctor --format json
venom build --config venom.toml --format json
```

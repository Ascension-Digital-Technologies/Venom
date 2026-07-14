# Protected modules

Protected modules group related exports, contracts, and dependencies behind one runtime boundary. They are appropriate for engines, policy modules, analytics, pricing, scoring, and other cohesive proprietary subsystems.

The compiler resolves supported local dependencies, transforms imports/exports, compiles the module graph to QuickJS bytecode, generates an opaque browser facade, and records a contract manifest for release verification.

See also the detailed legacy reference at [PROTECTED-MODULES.md](../PROTECTED-MODULES.md).

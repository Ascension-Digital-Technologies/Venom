# Phase 1A Residual Risk

The extraction implementation remains declaration- and heuristic-based rather than AST-based. Arrow functions, function expressions, methods, and other unsupported forms are rejected rather than extracted.

The `isolated` annotation still acts as an explicit trust assertion for named declarations after syntactic extraction succeeds. A later AST/lexical-scope pass should prove self-containment instead of relying on that assertion.

The intent ledger is not yet emitted as a standalone build-time JSON artifact, and verify does not independently consume such a ledger. The production compiler gate now enforces closure before package creation, which addresses the demonstrated leak path, but a durable intent-to-bytecode ledger remains the next hardening step.

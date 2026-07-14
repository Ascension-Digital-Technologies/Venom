# Venom v2 architecture preview

Venom v2 remains a local compiler and browser runtime. It does not add a server dependency, remote execution service, hosted build requirement, or online control plane.

## Planner precedence

The automatic protection planner accelerates project onboarding without taking control away from the developer. Decisions are resolved in this order:

1. `@venom` function and file annotations.
2. Explicit CLI `--protect` and `--browser` rules.
3. Manual `venom.toml` protection rules.
4. Automatic planner recommendations.
5. Protected-by-default fallback.

A higher-precedence decision cannot be overwritten by the planner. Conflicts are reported rather than silently resolved.

## Initial v2 foundations

- local protection-planning command;
- `standard`, `strong`, and `maximum` protection levels;
- manual include/exclude overrides available with every planner mode;
- machine-readable planning reports;
- no server-assisted or remote-execution path;
- compatibility with existing annotations and protected-by-default behavior.


## Function-level planning

Alpha.2 analyzes named function declarations and arrow functions independently. Each recommendation records the function name, source line, proposed realm, confidence, a protected-execution purity score, decision source, and human-readable reasons.

Dynamic source generation and unresolved runtime loading are never auto-approved. Strict mode fails until those findings are resolved with an annotation, CLI override, or configuration rule.

## Build enforcement

`venom build --planner recommend` prints a compact planning summary before compilation. `venom build --planner strict` rejects unresolved or low-confidence planner findings. Manual decisions remain authoritative in every mode.

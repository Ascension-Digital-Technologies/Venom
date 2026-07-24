from pathlib import Path
import json

root = Path(__file__).resolve().parents[2]
package = root / "packages/react"
pkg = json.loads((package / "package.json").read_text(encoding="utf-8"))
source = (package / "src/index.js").read_text(encoding="utf-8")
types = (package / "src/index.d.ts").read_text(encoding="utf-8")
example_pkg = json.loads((root / "examples/react-vite-showcase/package.json").read_text(encoding="utf-8"))
example_source = (root / "examples/react-vite-showcase/src/main.tsx").read_text(encoding="utf-8")

if pkg.get("name") != "@venom/react":
    raise SystemExit("React SDK package name is incorrect")
for peer in ("react", "@venom/runtime"):
    if peer not in pkg.get("peerDependencies", {}):
        raise SystemExit(f"React SDK is missing peer dependency: {peer}")
for token in (
    "useVenomRuntime", "useProtectedCall", "useProtectedPreload",
    "createProtectedHooks", "useSyncExternalStore", "RUNTIME_ABORTED"
):
    if token not in source:
        raise SystemExit(f"React SDK implementation is missing: {token}")
for token in ("ProtectedCallState", "ProtectedCallDefaults", "UseVenomRuntimeOptions"):
    if token not in types:
        raise SystemExit(f"React SDK types are missing: {token}")
if "@venom/react" not in example_pkg.get("dependencies", {}):
    raise SystemExit("React showcase does not depend on @venom/react")
if "useVenomRuntime" not in example_source:
    raise SystemExit("React showcase does not exercise the React runtime hook")
print("React hooks support smoke: PASS")

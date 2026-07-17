from pathlib import Path

root = Path(__file__).resolve().parents[2]
build_ps=(root/"scripts"/"windows"/"build-site.ps1").read_text(errors="ignore")
build_sh=(root/"scripts"/"linux"/"build-site.sh").read_text(errors="ignore")
checks = {
    "stale compiler prevention": "SkipCompilerBuild" in build_ps and "build.ps1" in build_ps and "rm -rf" in build_sh,
    "mandatory production gate": "verify" in build_ps and "verify-runtime" in build_ps and "verify" in build_sh and "verify-runtime" in build_sh,
    "opaque protected transport ids": "__venomInvokeProtectedById" in (root / "src/compiler/pipeline/js_rewriting.cpp").read_text(errors="ignore") and "__venomInvokeProtectedByName" not in (root / "src/compiler/pipeline/js_rewriting.cpp").read_text(errors="ignore"),
    "protected chess bytecode gate": "quickjs_bytecode_records" in (root / "tests/package/protected-chess-engine-smoke.py").read_text(errors="ignore"),
    "release leak scan": any("leak" in p.name for p in (root / "tests/package").glob("*leak*")),
    "native bytecode security test": (root / "tests/quickjs/native-bytecode-security.cpp").is_file(),
}
failed = [name for name, passed in checks.items() if not passed]
for name, passed in checks.items():
    print(f"[{'PASS' if passed else 'FAIL'}] {name}")
if failed:
    raise SystemExit("assurance checks failed: " + ", ".join(failed))
print("protected runtime assurance smoke passed")

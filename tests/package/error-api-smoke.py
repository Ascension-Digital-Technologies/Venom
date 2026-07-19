#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]
src = root / "src"
include = root / "include"
failures: list[str] = []
for path in sorted(src.rglob("*")):
    if not path.is_file() or path.suffix not in {".cpp", ".cc", ".cxx", ".hpp", ".h"}:
        continue
    if "generated" in path.relative_to(src).parts:
        continue
    text = path.read_text(encoding="utf-8", errors="replace")
    if re.search(r"\bthrow\s+std::(?:runtime_error|logic_error|invalid_argument|domain_error)\b", text):
        failures.append(f"{path.relative_to(root)}: direct standard exception throw")
    if "raise_error(" in text and path.suffix in {".cpp", ".cc", ".cxx"} and '#include "venom/base/error.hpp"' not in text:
        failures.append(f"{path.relative_to(root)}: raise_error without explicit base/error.hpp include")
    if "print_fatal_error" in text:
        failures.append(f"{path.relative_to(root)}: removed fatal-error compatibility API is still referenced")

error_header = (include / "venom/base/error.hpp").read_text(encoding="utf-8")
diagnostic_header = (include / "venom/core/diagnostic.hpp").read_text(encoding="utf-8")
main_source = (src / "cli/main.cpp").read_text(encoding="utf-8")
required = {
    "typed base error": "class Error : public std::runtime_error" in error_header,
    "stable exit taxonomy": "enum class ExitCode" in error_header,
    "diagnostics derive from base error": "class DiagnosticError final : public Error" in diagnostic_header,
    "CLI catches typed core errors": "catch (const venom::Error& error)" in main_source,
    "unexpected errors normalized": "print_unexpected_error" in main_source,
}
for label, ok in required.items():
    if not ok:
        failures.append(label)

if failures:
    raise SystemExit("error API smoke failed:\n  - " + "\n  - ".join(failures))
print("error API smoke passed")

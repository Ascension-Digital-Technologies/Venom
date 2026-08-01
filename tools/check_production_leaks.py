#!/usr/bin/env python3
"""Fail when a hardened Venom distribution exposes build/reverse-engineering metadata."""
from __future__ import annotations
import argparse
import re
import sys
from pathlib import Path

REPORT_NAMES = {
    "execution-plan.txt", "execution-plan.json", "function-plan.txt", "function-plan.json",
    "function-extraction-plan.txt", "function-extraction-plan.json",
    "runtime-bridge-contract.json", "bridge-rewrite-plan.json",
}
PUBLIC_METADATA = {
    "readme", "readme.md", "readme.txt", "changes", "changes.md", "changelog",
    "changelog.md", "contributing", "contributing.md", "security", "security.md",
}
TEXT_EXTENSIONS = {".js", ".json", ".html", ".css", ".txt", ".md", ".map"}
BYTECODE_EXTENSIONS = {".vqc", ".vbc", ".vqbc"}
# Production package sections must carry the permuted VTJSE006 envelope, never a
# bare canonical record (docs/security/protection-strengths.md, "TurboJS bytecode
# storage"). Any of these appearing un-enveloped in a shipped artifact means a
# delivery path bypassed that envelope step -- exactly how the loose .vqc asset
# in examples/protected-chess's browser build shipped a raw VTJSBC03 record.
UNENVELOPED_BYTECODE_MAGICS = (b"VTJSBC03", b"VTJSMB04")
BYTECODE_CONTAINER_MAGICS = (b"VTJSBC03", b"VTJSMB04", b"VENOMVBC", b"VTJSE006")
# Three-or-more-word lowerCamelCase identifiers (generatePseudoLegal,
# buildPrincipalVariation, computeHash, ...). Legitimate compiled bytecode/package
# records should carry none of these once source identifiers have been mangled;
# a cluster of them is the direct signature of the registry-hardening bug this
# scan exists to catch a regression of.
CAMEL_IDENTIFIER = re.compile(rb"\b[a-z][a-z0-9]{2,}(?:[A-Z][a-z0-9]{2,}){2,}\b")
CAMEL_IDENTIFIER_THRESHOLD = 12
FORBIDDEN_TEXT = (
    re.compile(rb"(?:^|[/'\"\\])(?:js|src)/[A-Za-z0-9_.\-/]+\.js"),
    re.compile(rb"function-extraction-plan|runtime-bridge-contract|bridge-rewrite-plan"),
    re.compile(rb"VENOM_TJS_RUNTIME_ABI_V\d+"),
    re.compile(rb"venom_tjs_[A-Za-z0-9_]+"),
    re.compile(rb"bridge-result|bridge-error|type:[\'\"](?:invoke|cancel)[\'\"]"),
)

def is_bytecode_artifact(path: Path, data: bytes) -> bool:
    if path.suffix.lower() in BYTECODE_EXTENSIONS:
        return True
    return any(data.startswith(magic) for magic in BYTECODE_CONTAINER_MAGICS)

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("dist", type=Path)
    args = ap.parse_args()
    dist = args.dist.resolve()
    if not dist.is_dir():
        print(f"error: distribution directory does not exist: {dist}", file=sys.stderr)
        return 2
    problems: list[str] = []
    for path in sorted(p for p in dist.rglob("*") if p.is_file()):
        rel = path.relative_to(dist).as_posix()
        low = path.name.lower()
        if path.name in REPORT_NAMES or "/build/reports/" in f"/{rel}":
            problems.append(f"forbidden production report: {rel}")
        if "/" not in rel and low in PUBLIC_METADATA:
            problems.append(f"root project metadata copied into distribution: {rel}")
        # Public assets should not contain descriptive bridge/WASM ABI or original source paths.
        data = None
        if path.suffix.lower() in TEXT_EXTENSIONS or path.suffix.lower() == ".wasm":
            data = path.read_bytes()
            for pattern in FORBIDDEN_TEXT:
                match = pattern.search(data)
                if match:
                    token = match.group(0)[:120].decode("utf-8", "replace")
                    problems.append(f"metadata leak in {rel}: {token}")
                    break
            if path.suffix.lower() == ".js":
                for marker in (b"VAEAD001", b"VSEAL001", b"VSODIUM1", b"deriveAeadStreamSeed", b"xorSealStream"):
                    if marker in data:
                        problems.append(f"browser package decoder leak in {rel}: {marker.decode('ascii')}")
                        break
        # A shipped bytecode/package artifact must never expose a bare
        # VTJSBC03/VTJSMB04 magic (production sections carry VTJSE006 instead),
        # and must not carry a dense cluster of readable multi-word identifiers --
        # the direct fingerprint of unmangled source reaching compiled output.
        # Scoped to files that are themselves bytecode-shaped artifacts, not any
        # file that merely mentions the magic string: the runtime loader and the
        # WASM interpreter legitimately contain "VTJSBC03" as a literal they
        # validate incoming records against.
        if data is None:
            data = path.read_bytes()
        if is_bytecode_artifact(path, data):
            for magic in UNENVELOPED_BYTECODE_MAGICS:
                if data.startswith(magic):
                    problems.append(f"unenveloped bytecode record in {rel}: {magic.decode('ascii')}")
                    break
            identifiers = sorted(set(CAMEL_IDENTIFIER.findall(data)))
            if len(identifiers) >= CAMEL_IDENTIFIER_THRESHOLD:
                sample = ", ".join(m.decode("ascii", "replace") for m in identifiers[:6])
                problems.append(
                    f"readable identifier cluster in {rel}: {len(identifiers)} distinct names, e.g. {sample}"
                )
    if problems:
        print("production leak scan: FAIL", file=sys.stderr)
        for problem in problems:
            print(f"  - {problem}", file=sys.stderr)
        return 1
    print("production leak scan: PASS")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

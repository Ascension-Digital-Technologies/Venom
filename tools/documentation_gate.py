#!/usr/bin/env python3
"""Repository documentation consistency and publication gate."""
from __future__ import annotations
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REQUIRED = [
    "README.md", "docs/README.md", "SECURITY.md", "SUPPORT.md", "CONTRIBUTING.md",
    "CODE_OF_CONDUCT.md", "GOVERNANCE.md", "ROADMAP.md", "CITATION.cff",
    "docs/getting-started/installation.md", "docs/getting-started/existing-project.md",
    "docs/guides/annotations.md", "docs/guides/browser-bridge.md",
    "docs/architecture/protected-runtime.md", "docs/security/security-model.md",
    "docs/security/limitations.md", "docs/reference/cli.md", "docs/MAINTENANCE.md",
    "examples/protected-chess/README.md", "examples/nova-trade/README.md", "examples/bot-detection/README.md",
]
FORBIDDEN_CLAIMS = [
    r"\bunbreakable\b", r"\bimpossible to reverse\b", r"\bmilitary[- ]grade\b",
    r"\b100% secure\b", r"\bfully encrypted in the browser\b",
]
LINK_RE = re.compile(r"\[[^\]]+\]\(([^)]+)\)")
MERMAID_RE = re.compile(r"```mermaid\s*\n(.*?)```", re.DOTALL)
SEQUENCE_PARTICIPANT_RE = re.compile(r'^\s*participant\s+[A-Za-z_][A-Za-z0-9_]*(?:\s+as\s+"[^"\n]+")?\s*$')
SEQUENCE_MESSAGE_RE = re.compile(r'^\s*[A-Za-z_][A-Za-z0-9_]*\s*(?:-+|=+)(?:>>|>)[A-Za-z_][A-Za-z0-9_]*\s*:\s*\S.*$')

def fail(msg: str) -> None:
    print(f"[FAIL] {msg}")
    raise SystemExit(1)

def main() -> int:
    for rel in REQUIRED:
        if not (ROOT / rel).is_file():
            fail(f"missing required documentation: {rel}")

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    m = re.search(r"VERSION\s+(\d+\.\d+\.\d+)", cmake)
    if not m:
        fail("unable to determine CMake project version")
    version = m.group(1)
    obsolete_rc_reports = list((ROOT / "docs/development").glob("release-candidate-*-report.md"))
    if obsolete_rc_reports:
        fail("stable source package contains obsolete release-candidate reports: " + ", ".join(str(p.relative_to(ROOT)) for p in obsolete_rc_reports))

    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    if version not in readme:
        fail(f"README does not mention current version {version}")

    markdown = list(ROOT.glob("*.md")) + list((ROOT / "docs").rglob("*.md")) + list((ROOT / "examples").glob("*/README.md"))
    for path in markdown:
        text = path.read_text(encoding="utf-8")
        lowered = text.lower()
        for pattern in FORBIDDEN_CLAIMS:
            if re.search(pattern, lowered):
                fail(f"unsupported security claim in {path.relative_to(ROOT)}: {pattern}")
        if text.count("```mermaid") != len(MERMAID_RE.findall(text)):
            fail(f"unterminated Mermaid block in {path.relative_to(ROOT)}")
        for block in MERMAID_RE.findall(text):
            lines = [line.rstrip() for line in block.strip().splitlines() if line.strip()]
            if not lines:
                fail(f"empty Mermaid block in {path.relative_to(ROOT)}")
            diagram_type = lines[0].strip()
            if diagram_type not in {"sequenceDiagram", "flowchart LR", "flowchart TD", "flowchart TB", "flowchart RL"}:
                fail(f"unsupported Mermaid diagram declaration in {path.relative_to(ROOT)}: {diagram_type}")
            if diagram_type == "sequenceDiagram":
                participants = set()
                for line in lines[1:]:
                    stripped = line.strip()
                    if stripped.startswith("participant "):
                        if not SEQUENCE_PARTICIPANT_RE.match(line):
                            fail(f"invalid Mermaid participant in {path.relative_to(ROOT)}: {stripped}")
                        participants.add(stripped.split()[1])
                        continue
                    if stripped.startswith(("Note ", "activate ", "deactivate ", "loop ", "alt ", "else", "end")):
                        continue
                    if not SEQUENCE_MESSAGE_RE.match(line):
                        fail(f"invalid Mermaid sequence message in {path.relative_to(ROOT)}: {stripped}")
                    endpoints = re.split(r"(?:-+|=+)(?:>>|>)", stripped.split(":", 1)[0])
                    if len(endpoints) != 2 or any(endpoint.strip() not in participants for endpoint in endpoints):
                        fail(f"Mermaid message references undeclared participant in {path.relative_to(ROOT)}: {stripped}")

        for target in LINK_RE.findall(text):
            if target.startswith(("http://", "https://", "mailto:", "#")):
                continue
            clean = target.split("#", 1)[0]
            if not clean:
                continue
            resolved = (path.parent / clean).resolve()
            try:
                resolved.relative_to(ROOT.resolve())
            except ValueError:
                fail(f"link escapes repository in {path.relative_to(ROOT)}: {target}")
            if not resolved.exists():
                fail(f"broken local link in {path.relative_to(ROOT)}: {target}")

    if len((ROOT / "examples/nova-trade/README.md").read_text(encoding="utf-8").splitlines()) < 80:
        fail("NOVA TRADE README is not sufficiently detailed")
    if len((ROOT / "README.md").read_text(encoding="utf-8").splitlines()) < 200:
        fail("root README is unexpectedly short")

    print(f"[PASS] documentation gate: version={version} files={len(markdown)}")
    return 0

if __name__ == "__main__":
    sys.exit(main())

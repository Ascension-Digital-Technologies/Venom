#!/usr/bin/env python3
"""Repository documentation consistency and publication gate."""
from __future__ import annotations
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REQUIRED = [
    "README.md", "docs/README.md", "SECURITY.md", "SUPPORT.md",
    "docs/getting-started/installation.md", "docs/getting-started/build-from-source.md",
    "docs/getting-started/existing-project.md", "docs/getting-started/deployment.md",
    "docs/guides/annotations.md", "docs/guides/protected-functions.md",
    "docs/guides/protected-modules.md", "docs/guides/browser-bridge.md",
    "docs/architecture/overview.md", "docs/architecture/protected-runtime.md",
    "docs/architecture/package-format.md", "docs/security/security-model.md",
    "docs/security/threat-model.md", "docs/security/limitations.md",
    "docs/security/protection-strengths.md", "docs/security/production-hardening.md",
    "docs/operations/build-profiles.md", "docs/operations/release-verification.md",
    "docs/operations/release-packaging.md", "docs/operations/browser-equivalence.md",
    "docs/reference/cli.md", "docs/reference/javascript-api.md",
    "examples/protected-chess/README.md", "examples/nova-trade/README.md", "examples/bot-detection/README.md", "examples/typescript-showcase/README.md", "examples/tsx-showcase/README.md", "examples/aegis-operations/README.md", "examples/javascript-playground/README.md",
    "docs/assets/examples/protected-chess/application.png",
    "docs/assets/examples/nova-trade/terminal.png",
    "docs/assets/examples/bot-detection/overview.png",
]
FORBIDDEN_CLAIMS = [
    r"\bunbreakable\b", r"\bimpossible to reverse\b", r"\bmilitary[- ]grade\b",
    r"\b100% secure\b", r"\bfully encrypted in the browser\b",
]
LINK_RE = re.compile(r"\[[^\]]+\]\(([^)]+)\)")
MERMAID_RE = re.compile(r"```mermaid\s*\n(.*?)```", re.DOTALL)
SEQUENCE_PARTICIPANT_RE = re.compile(r'^\s*participant\s+[A-Za-z_][A-Za-z0-9_]*(?:\s+as\s+(?:"[^"\n]+"|[^\n]+))?\s*$')
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
    forbidden_doc_paths = [
        ROOT / "docs/development", ROOT / "docs/MAINTENANCE.md", ROOT / "docs/STYLE-GUIDE.md",
        ROOT / "GOVERNANCE.md",
        ROOT / "ROADMAP.md", ROOT / "CITATION.cff",
    ]
    present_forbidden = [str(p.relative_to(ROOT)) for p in forbidden_doc_paths if p.exists()]
    if present_forbidden:
        fail("production source package contains development-process documentation: " + ", ".join(present_forbidden))
    allowed_doc_dirs = {"getting-started", "guides", "architecture", "security", "operations", "reference", "generated", "assets", "audit"}
    unexpected = []
    for child in (ROOT / "docs").iterdir():
        if child.name == "README.md":
            continue
        if child.is_dir() and child.name in allowed_doc_dirs:
            continue
        unexpected.append(str(child.relative_to(ROOT)))
    if unexpected:
        fail("documentation is outside the production folder structure: " + ", ".join(unexpected))

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
    required_strength_markers = [
        "## Why a Venom distribution is difficult to reverse",
        "exact original protected source is not present",
        "Polymorphic builds",
        "VQJSE006",
        "Private binary capability bridge",
        "WebAssembly memory remains inspectable",
    ]
    for marker in required_strength_markers:
        if marker not in readme:
            fail(f"root README is missing protection-strength marker: {marker}")

    detailed_docs = {
        "docs/getting-started/build-from-source.md": 80,
        "docs/operations/release-verification.md": 80,
        "docs/architecture/package-format.md": 150,
        "docs/security/security-model.md": 70,
        "docs/security/protection-strengths.md": 60,
        "docs/README.md": 100,
    }
    for rel, minimum in detailed_docs.items():
        count = len((ROOT / rel).read_text(encoding="utf-8").splitlines())
        if count < minimum:
            fail(f"production documentation is unexpectedly thin: {rel} ({count} < {minimum})")

    print(f"[PASS] documentation gate: version={version} files={len(markdown)}")
    return 0

if __name__ == "__main__":
    sys.exit(main())

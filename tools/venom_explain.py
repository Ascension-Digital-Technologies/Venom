#!/usr/bin/env python3
"""Explain Venom runtime ownership, module dependencies, and build evidence."""
from __future__ import annotations
import argparse, hashlib, json, os, re, sys
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Any

EXTS = (".js", ".mjs", ".cjs", ".jsx", ".ts", ".mts", ".cts", ".tsx")
DIRECTIVE_RE = re.compile(r"@venom\s*:\s*(browser|protected)\b", re.I)
IMPORT_RE = re.compile(r"(?:import\s+(?:[^'\"]*?\s+from\s+)?|export\s+[^'\"]*?\s+from\s+|import\s*\()\s*['\"]([^'\"]+)['\"]", re.M)
REQUIRE_RE = re.compile(r"\brequire\s*\(\s*['\"]([^'\"]+)['\"]\s*\)")
SKIP_DIRS = {"node_modules", ".git", ".venom", ".venom-cache", "build", "dist"}

@dataclass
class Diagnostic:
    code: str
    severity: str
    message: str
    file: str | None = None
    dependency: str | None = None
    remediation: str | None = None


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def load_contract(root: Path) -> dict[str, Any]:
    path = root / "contracts" / "diagnostics.json"
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema") != "VENOM_DIAGNOSTICS_CONTRACT_V1":
        raise RuntimeError("VENOM-D1401: invalid diagnostics contract schema")
    return data


def source_files(site: Path) -> list[Path]:
    out: list[Path] = []
    for p in site.rglob("*"):
        if not p.is_file() or p.suffix.lower() not in EXTS:
            continue
        rel_parts = p.relative_to(site).parts
        if any(part in SKIP_DIRS or part.startswith("dist-") or part.startswith("build-") for part in rel_parts):
            continue
        out.append(p)
    return sorted(out)


def detect_runtime(text: str) -> tuple[str, list[str]]:
    head = "\n".join(text.splitlines()[:20])
    found = [m.group(1).lower() for m in DIRECTIVE_RE.finditer(head)]
    unique = sorted(set(found))
    if len(unique) > 1:
        return "conflict", unique
    return (unique[0], unique) if unique else ("unspecified", [])


def resolve_relative(src: Path, spec: str) -> Path | None:
    base = (src.parent / spec)
    candidates: list[Path] = []
    if base.suffix:
        candidates.append(base)
    else:
        candidates.extend(Path(str(base) + ext) for ext in EXTS)
        candidates.extend(base / ("index" + ext) for ext in EXTS)
    for c in candidates:
        if c.is_file():
            return c.resolve()
    return None


def discover_report(site: Path, filename: str) -> Path | None:
    direct = site / filename
    if direct.is_file(): return direct
    candidates = sorted(site.glob(f".venom/**/{filename}"), key=lambda p: p.stat().st_mtime, reverse=True)
    return candidates[0] if candidates else None


def summarize_performance(data: dict[str, Any]) -> dict[str, Any]:
    phases = data.get("phases", [])
    normalized = []
    if isinstance(phases, dict):
        normalized = [{"name": k, "durationMs": v} for k, v in phases.items()]
    elif isinstance(phases, list):
        normalized = phases
    normalized = sorted(normalized, key=lambda x: float(x.get("duration_ms", x.get("durationMs", 0)) or 0), reverse=True)
    return {"elapsedMs": data.get("elapsed_ms", data.get("elapsedMs")), "slowestPhases": normalized[:5], "cache": data.get("cache", {})}


def analyze(site: Path, repo_root: Path, build_report: Path | None, perf_report: Path | None) -> dict[str, Any]:
    contract = load_contract(repo_root)
    codes = contract["codes"]
    files = source_files(site)
    records: dict[Path, dict[str, Any]] = {}
    diagnostics: list[Diagnostic] = []
    for path in files:
        text = path.read_text(encoding="utf-8", errors="replace")
        runtime, directives = detect_runtime(text)
        rel = path.relative_to(site).as_posix()
        if runtime == "unspecified":
            spec = codes["VENOM-D1001"]
            diagnostics.append(Diagnostic("VENOM-D1001", spec["severity"], f"{rel} has no explicit file-scope runtime directive", rel, remediation=spec["remediation"]))
        elif runtime == "conflict":
            spec = codes["VENOM-D1002"]
            diagnostics.append(Diagnostic("VENOM-D1002", spec["severity"], f"{rel} declares conflicting runtimes: {', '.join(directives)}", rel, remediation=spec["remediation"]))
        specs = IMPORT_RE.findall(text) + REQUIRE_RE.findall(text)
        records[path.resolve()] = {"file": rel, "runtime": runtime, "sha256": sha256_file(path), "dependencies": [], "dependents": []}
        records[path.resolve()]["rawSpecs"] = specs

    edges = []
    for path, rec in records.items():
        src = Path(path)
        for specifier in rec.pop("rawSpecs"):
            edge: dict[str, Any] = {"from": rec["file"], "specifier": specifier, "kind": "external"}
            if specifier.startswith("."):
                resolved = resolve_relative(src, specifier)
                edge["kind"] = "relative"
                if resolved is None or resolved not in records:
                    code = codes["VENOM-D1101"]
                    diagnostics.append(Diagnostic("VENOM-D1101", code["severity"], f"{rec['file']} cannot resolve {specifier}", rec["file"], specifier, code["remediation"]))
                    edge["resolved"] = None
                else:
                    target = records[resolved]
                    edge["resolved"] = target["file"]
                    edge["targetRuntime"] = target["runtime"]
                    rec["dependencies"].append(target["file"])
                    target["dependents"].append(rec["file"])
                    if rec["runtime"] == "browser" and target["runtime"] == "protected":
                        code = codes["VENOM-D1102"]
                        diagnostics.append(Diagnostic("VENOM-D1102", code["severity"], f"Browser module {rec['file']} crosses into protected module {target['file']}", rec["file"], specifier, code["remediation"]))
            edges.append(edge)

    evidence: dict[str, Any] = {}
    if build_report is None: build_report = discover_report(site, "build-report.json")
    if perf_report is None: perf_report = discover_report(site, "build-performance.json")
    if build_report and build_report.is_file():
        evidence["buildReport"] = {"path": str(build_report), "sha256": sha256_file(build_report), "data": json.loads(build_report.read_text(encoding="utf-8"))}
    if perf_report and perf_report.is_file():
        pdata = json.loads(perf_report.read_text(encoding="utf-8"))
        evidence["performanceReport"] = {"path": str(perf_report), "sha256": sha256_file(perf_report), "summary": summarize_performance(pdata)}
    if not evidence:
        code = codes["VENOM-D1201"]
        diagnostics.append(Diagnostic("VENOM-D1201", code["severity"], "No build or performance evidence was found", remediation=code["remediation"]))

    counts = {"browser": 0, "protected": 0, "unspecified": 0, "conflict": 0}
    for rec in records.values(): counts[rec["runtime"]] = counts.get(rec["runtime"], 0) + 1
    sev = {"info": 0, "warning": 0, "error": 0}
    for d in diagnostics: sev[d.severity] = sev.get(d.severity, 0) + 1
    return {
        "schema": "VENOM_EXPLAIN_REPORT_V1",
        "site": str(site.resolve()),
        "contract": {"schema": contract["schema"], "version": contract["version"], "sha256": sha256_file(repo_root / "contracts" / "diagnostics.json")},
        "summary": {"modules": len(records), "edges": len(edges), "runtimes": counts, "diagnostics": sev},
        "modules": sorted(records.values(), key=lambda x: x["file"]),
        "edges": edges,
        "diagnostics": [asdict(d) for d in diagnostics],
        "evidence": evidence
    }


def dot_graph(report: dict[str, Any]) -> str:
    runtime_color = {"browser": "lightskyblue", "protected": "lightcoral", "unspecified": "lightgray", "conflict": "gold"}
    lines = ["digraph venom_modules {", "  rankdir=LR;", "  node [shape=box, style=filled, fontname=\"Arial\"];" ]
    for module in report["modules"]:
        name = module["file"].replace("\\", "\\\\").replace('"', '\\"')
        color = runtime_color.get(module["runtime"], "white")
        lines.append(f'  "{name}" [fillcolor="{color}", label="{name}\\n[{module["runtime"]}]"];')
    for edge in report["edges"]:
        if edge.get("resolved"):
            src = edge["from"].replace('"', '\\"')
            dst = edge["resolved"].replace('"', '\\"')
            lines.append(f'  "{src}" -> "{dst}";')
    lines.append("}")
    return "\n".join(lines) + "\n"

def markdown(report: dict[str, Any]) -> str:
    s = report["summary"]
    lines = ["# Venom Explain Report", "", f"Site: `{report['site']}`", "", "## Summary", "", f"- Modules: **{s['modules']}**", f"- Dependency edges: **{s['edges']}**", f"- Browser modules: **{s['runtimes'].get('browser',0)}**", f"- Protected modules: **{s['runtimes'].get('protected',0)}**", f"- Unspecified modules: **{s['runtimes'].get('unspecified',0)}**", f"- Errors: **{s['diagnostics'].get('error',0)}**", f"- Warnings: **{s['diagnostics'].get('warning',0)}**", "", "## Modules", "", "| Module | Runtime | Dependencies |", "|---|---|---:|"]
    for m in report["modules"]: lines.append(f"| `{m['file']}` | {m['runtime']} | {len(m['dependencies'])} |")
    lines += ["", "## Diagnostics", ""]
    if not report["diagnostics"]: lines.append("No diagnostics.")
    else:
        for d in report["diagnostics"]:
            loc = f" (`{d['file']}`)" if d.get("file") else ""
            lines += [f"### {d['code']} — {d['severity'].upper()}{loc}", "", d["message"], "", f"**Remediation:** {d.get('remediation') or 'Review the related contract.'}", ""]
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("site", type=Path)
    ap.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument("--build-report", type=Path)
    ap.add_argument("--performance-report", type=Path)
    ap.add_argument("--json-out", type=Path)
    ap.add_argument("--markdown-out", type=Path)
    ap.add_argument("--dot-out", type=Path, help="write a Graphviz module graph")
    ap.add_argument("--format", choices=("text", "json"), default="text")
    ap.add_argument("--fail-on", choices=("never", "warning", "error"), default="error")
    args = ap.parse_args()
    site = args.site.resolve()
    if not site.is_dir(): raise SystemExit(f"site directory not found: {site}")
    report = analyze(site, args.repo_root.resolve(), args.build_report, args.performance_report)
    payload = json.dumps(report, indent=2, sort_keys=True) + "\n"
    md = markdown(report)
    if args.json_out: args.json_out.parent.mkdir(parents=True, exist_ok=True); args.json_out.write_text(payload, encoding="utf-8")
    if args.markdown_out: args.markdown_out.parent.mkdir(parents=True, exist_ok=True); args.markdown_out.write_text(md, encoding="utf-8")
    if args.dot_out: args.dot_out.parent.mkdir(parents=True, exist_ok=True); args.dot_out.write_text(dot_graph(report), encoding="utf-8")
    print(payload if args.format == "json" else md, end="")
    sev = report["summary"]["diagnostics"]
    if args.fail_on == "error" and sev.get("error", 0): return 2
    if args.fail_on == "warning" and (sev.get("warning", 0) or sev.get("error", 0)): return 2
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

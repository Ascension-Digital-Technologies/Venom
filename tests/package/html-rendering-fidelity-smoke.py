#!/usr/bin/env python3
from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path


def run(command: list[str]) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
    if result.returncode != 0:
        raise RuntimeError(f"command failed ({result.returncode}): {' '.join(command)}\n{result.stdout}")
    return result


def main() -> int:
    if len(sys.argv) != 4:
        print("usage: html-rendering-fidelity-smoke.py <venom> <work> <node>", file=sys.stderr)
        return 2

    venom = Path(sys.argv[1]).resolve()
    work = Path(sys.argv[2]).resolve()
    node = Path(sys.argv[3]).resolve()
    source_root = Path(__file__).resolve().parents[2]
    harness = source_root / "tests" / "runtime" / "browser-compat-harness.mjs"
    site = work / "site"
    dist = work / "dist"
    shutil.rmtree(work, ignore_errors=True)
    site.mkdir(parents=True)

    (site / "base.css").write_text(".from-file { color: rgb(1, 2, 3); }\n", encoding="utf-8")
    (site / "index.html").write_text(
        "<!doctype html><html lang=\"en-GB\" class=\"document-class\"><head>"
        "<meta charset=\"utf-8\"><title>Rendering Fidelity</title>"
        "<style>table { border: 3px solid black; } td.passed { background: #c8d86d; }</style>"
        "<link rel=\"stylesheet\" href=\"./base.css\">"
        "</head><body class=\"source-body\" style=\"margin: 7px\">"
        "<p id=\"spacing\">User Agent <span>(Old)</span></p>"
        "<h1 id=\"heading\"><a href=\"#\">Fingerprint Scanner</a> tests</h1>"
        "<pre id=\"preformatted\">{\n  &quot;value&quot;: 1\n}</pre>"
        "<script>globalThis.__venomFidelityProbe = 1;</script>"
        "</body></html>",
        encoding="utf-8",
    )

    run([str(venom), "build", str(site), "--out", str(dist)])
    index = (dist / "index.html").read_text(encoding="utf-8")
    if '<title>Rendering Fidelity</title>' not in index:
        raise RuntimeError("source document title was not preserved")
    if '<html lang="en-GB" class="document-class">' not in index:
        raise RuntimeError("source html attributes were not preserved")
    if '<body class="source-body" style="margin: 7px">' not in index:
        raise RuntimeError("source body attributes were not preserved")

    styles = list((dist / "assets" / "style").glob("s.*.css"))
    if len(styles) != 1:
        raise RuntimeError("expected one production stylesheet")
    css = styles[0].read_text(encoding="utf-8")
    required = [
        "table { border: 3px solid black; }",
        "td.passed { background: #c8d86d; }",
        ".from-file { color: rgb(1, 2, 3); }",
    ]
    for marker in required:
        if marker not in css:
            raise RuntimeError(f"missing source CSS marker: {marker}")
    if "font-family:system-ui" in css or "margin:2rem" in css:
        raise RuntimeError("Venom injected visual defaults into a production site")
    if css.index("table { border: 3px solid black; }") > css.index(".from-file { color: rgb(1, 2, 3); }"):
        raise RuntimeError("document CSS cascade order was not preserved")

    boot = run([
        str(node), str(harness), str(dist), "strict-no-eval",
        "--strict-no-source-eval", "--via-loader", "--assert-html-fidelity", "--assert-malformed-html",
    ])
    if "browser compatibility harness passed: strict-no-eval:via-loader:html-fidelity:malformed-html" not in boot.stdout:
        raise RuntimeError(f"HTML fidelity boot assertions did not complete\n{boot.stdout}")

    print("html rendering fidelity smoke: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

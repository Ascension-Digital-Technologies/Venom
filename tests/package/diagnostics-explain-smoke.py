#!/usr/bin/env python3
import json, subprocess, sys, tempfile
from pathlib import Path
root = Path(__file__).resolve().parents[2]
tool = root / "tools" / "venom_explain.py"
contract = json.loads((root / "contracts" / "diagnostics.json").read_text())
assert contract["schema"] == "VENOM_DIAGNOSTICS_CONTRACT_V1"
with tempfile.TemporaryDirectory() as td:
    site = Path(td) / "site"; site.mkdir()
    (site / "app.ts").write_text('// @venom: browser\nimport { score } from "./score";\nconsole.log(score());\n')
    (site / "score.ts").write_text('// @venom: protected\nexport function score(){ return 42; }\n')
    jout = Path(td)/"report.json"; mout = Path(td)/"report.md"; dout = Path(td)/"graph.dot"
    run = subprocess.run([sys.executable, str(tool), str(site), "--repo-root", str(root), "--json-out", str(jout), "--markdown-out", str(mout), "--dot-out", str(dout), "--fail-on", "never"], text=True, capture_output=True)
    assert run.returncode == 0, run.stderr
    report = json.loads(jout.read_text())
    assert report["schema"] == "VENOM_EXPLAIN_REPORT_V1"
    assert report["summary"]["modules"] == 2
    assert report["summary"]["runtimes"]["browser"] == 1
    assert report["summary"]["runtimes"]["protected"] == 1
    assert any(d["code"] == "VENOM-D1102" for d in report["diagnostics"])
    assert "Venom Explain Report" in mout.read_text()
    assert "digraph venom_modules" in dout.read_text()
    assert "app.ts" in dout.read_text()
print("diagnostics explain smoke: passed")

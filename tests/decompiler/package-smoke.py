#!/usr/bin/env python3
import json
import pathlib
import shutil
import subprocess
import sys


def run(*args: str) -> None:
    result = subprocess.run(args, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode != 0:
        raise RuntimeError(f"command failed ({result.returncode}): {' '.join(args)}\n{result.stdout}")


def main() -> int:
    if len(sys.argv) != 4:
        raise SystemExit("usage: package-smoke.py <venom> <venom-decompiler> <work-dir>")
    venom = pathlib.Path(sys.argv[1]).resolve()
    standalone = pathlib.Path(sys.argv[2]).resolve()
    work = pathlib.Path(sys.argv[3]).resolve()
    shutil.rmtree(work, ignore_errors=True)
    site = work / "site"
    dist = work / "dist"
    recovered = work / "recovered"
    standalone_recovered = work / "standalone-recovered"
    (site / "assets" / "js").mkdir(parents=True)
    (site / "index.html").write_text(
        "<!doctype html><html><head><meta charset='utf-8'><title>Recovery smoke</title></head>"
        "<body><main id='app' data-test='deob'><h1>Recovery smoke</h1><button id='run'>Run</button>"
        "<output id='result'>idle</output></main><script src='assets/js/app.js'></script></body></html>",
        encoding="utf-8",
    )
    (site / "assets" / "js" / "app.js").write_text(
        "// @venom: browser\n"
        "const values=[3,7,11];document.getElementById('run').addEventListener('click',()=>{"
        "document.getElementById('result').textContent=String(values.reduce((a,b)=>a+b,0));});\n",
        encoding="utf-8",
    )

    run(str(venom), "build", str(site), "--out", str(dist), "--profile", "dev", "--seed", "424242")
    run(str(venom), "decompile", str(dist), "--out", str(recovered), "--force", "--format", "json")

    report = json.loads((recovered / "recovery-report.json").read_text(encoding="utf-8"))
    assert report["recovery"]["decodedSections"] > 0
    assert report["recovery"]["routes"] == 1
    assert report["recovery"]["vmInstructions"] > 0
    assert report["recovery"]["scriptBundles"] == 1
    assert report["recovery"]["browserSources"] == 1
    assert report["limits"]["doesNotExecuteRecoveredCode"] is True
    reconstructed = next((recovered / "route-vm").glob("route-*/reconstructed.html"))
    html = reconstructed.read_text(encoding="utf-8")
    assert '<main id="app" data-test="deob">' in html
    assert "Recovery smoke" in html
    readable_scripts = list((recovered / "scripts").glob("**/browser-source.readable.js"))
    assert len(readable_scripts) == 1
    assert "addEventListener" in readable_scripts[0].read_text(encoding="utf-8")

    package = next((dist / "assets" / "app").glob("*.vbc"))
    run(str(standalone), str(package), "--out", str(standalone_recovered), "--force")
    assert (standalone_recovered / "sections" / "index.json").exists()
    assert (standalone_recovered / "route-vm" / "routes.json").exists()

    nested_recovery = site / "recovered-inside-input"
    run(str(venom), "decompile", str(site), "--out", str(nested_recovery), "--force", "--format", "json")
    nested_report = json.loads((nested_recovery / "recovery-report.json").read_text(encoding="utf-8"))
    assert nested_report["recovery"]["looseJavaScriptFiles"] == 1
    assert len(list((nested_recovery / "javascript").glob("**/*.readable.js"))) == 1
    print("venom decompiler package smoke passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

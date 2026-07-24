#!/usr/bin/env python3
"""Minimal dependency-free Venom language server over LSP stdio."""
from __future__ import annotations
import argparse, json, sys, urllib.parse
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))
import venom_explain as explain

SEVERITY = {"error": 1, "warning": 2, "info": 3}


def uri_to_path(uri: str) -> Path:
    return Path(urllib.parse.unquote(urllib.parse.urlparse(uri).path.lstrip('/') if sys.platform == 'win32' else urllib.parse.urlparse(uri).path))


def path_to_uri(path: Path) -> str:
    return path.resolve().as_uri()


def send(payload: dict[str, Any]) -> None:
    raw = json.dumps(payload, separators=(",", ":")).encode()
    sys.stdout.buffer.write(f"Content-Length: {len(raw)}\r\n\r\n".encode() + raw)
    sys.stdout.buffer.flush()


def read_message() -> dict[str, Any] | None:
    length = None
    while True:
        line = sys.stdin.buffer.readline()
        if not line:
            return None
        if line in (b"\r\n", b"\n"):
            break
        key, _, value = line.decode(errors="replace").partition(":")
        if key.lower() == "content-length":
            length = int(value.strip())
    if length is None:
        return None
    return json.loads(sys.stdin.buffer.read(length))


def line_of(text: str, needle: str) -> int:
    pos = text.find(needle)
    return 0 if pos < 0 else text.count("\n", 0, pos)


def document_diagnostics(path: Path, text: str) -> list[dict[str, Any]]:
    root = Path(__file__).resolve().parents[1]
    contract = explain.load_contract(root)["codes"]
    runtime, directives = explain.detect_runtime(text)
    out: list[dict[str, Any]] = []
    def add(code: str, message: str, line: int = 0) -> None:
        spec = contract[code]
        out.append({"range":{"start":{"line":line,"character":0},"end":{"line":line,"character":max(1,len(text.splitlines()[line]) if text.splitlines() and line < len(text.splitlines()) else 1)}},"severity":SEVERITY[spec["severity"]],"code":code,"source":"venom","message":message,"codeDescription":{"href":f"https://venom.dev/diagnostics/{code}"},"data":{"remediation":spec["remediation"]}})
    if runtime == "unspecified": add("VENOM-D1001", "Module has no explicit file-scope Venom runtime directive.")
    elif runtime == "conflict": add("VENOM-D1002", f"Module declares conflicting runtimes: {', '.join(directives)}")
    for specifier in explain.IMPORT_RE.findall(text) + explain.REQUIRE_RE.findall(text):
        if specifier.startswith('.') and explain.resolve_relative(path, specifier) is None:
            add("VENOM-D1101", f"Relative dependency cannot be resolved: {specifier}", line_of(text, specifier))
    return out


def hover(path: Path, text: str) -> dict[str, Any]:
    runtime, _ = explain.detect_runtime(text)
    deps = explain.IMPORT_RE.findall(text) + explain.REQUIRE_RE.findall(text)
    return {"contents":{"kind":"markdown","value":f"**Venom runtime:** `{runtime}`\n\n**Dependencies:** {len(deps)}\n\nUse `venom_explain.py` for the complete module graph."}}


def main() -> int:
    argparse.ArgumentParser(description=__doc__).parse_args()
    docs: dict[str, str] = {}
    shutdown = False
    while True:
        msg = read_message()
        if msg is None: break
        method, ident, params = msg.get("method"), msg.get("id"), msg.get("params", {})
        if method == "initialize":
            send({"jsonrpc":"2.0","id":ident,"result":{"capabilities":{"textDocumentSync":1,"hoverProvider":True,"codeActionProvider":True},"serverInfo":{"name":"venom-lsp","version":"3.0.0"}}})
        elif method == "initialized": pass
        elif method == "shutdown": shutdown = True; send({"jsonrpc":"2.0","id":ident,"result":None})
        elif method == "exit": return 0 if shutdown else 1
        elif method in ("textDocument/didOpen", "textDocument/didChange"):
            td = params["textDocument"]; uri = td["uri"]
            text = td.get("text") if method.endswith("didOpen") else params["contentChanges"][-1]["text"]
            docs[uri] = text
            send({"jsonrpc":"2.0","method":"textDocument/publishDiagnostics","params":{"uri":uri,"diagnostics":document_diagnostics(uri_to_path(uri), text)}})
        elif method == "textDocument/didClose":
            uri = params["textDocument"]["uri"]; docs.pop(uri, None)
            send({"jsonrpc":"2.0","method":"textDocument/publishDiagnostics","params":{"uri":uri,"diagnostics":[]}})
        elif method == "textDocument/hover":
            uri = params["textDocument"]["uri"]; path = uri_to_path(uri); text = docs.get(uri, path.read_text(encoding='utf-8', errors='replace') if path.exists() else '')
            send({"jsonrpc":"2.0","id":ident,"result":hover(path,text)})
        elif method == "textDocument/codeAction":
            uri = params["textDocument"]["uri"]
            actions=[]
            for d in params.get("context",{}).get("diagnostics",[]):
                if d.get("code") == "VENOM-D1001":
                    for runtime in ("browser","protected"):
                        actions.append({"title":f"Mark module as Venom {runtime}","kind":"quickfix","diagnostics":[d],"edit":{"changes":{uri:[{"range":{"start":{"line":0,"character":0},"end":{"line":0,"character":0}},"newText":f"// @venom: {runtime}\n"}]}}})
            send({"jsonrpc":"2.0","id":ident,"result":actions})
        elif ident is not None:
            send({"jsonrpc":"2.0","id":ident,"result":None})
    return 0

if __name__ == '__main__': raise SystemExit(main())

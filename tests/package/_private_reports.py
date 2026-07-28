from __future__ import annotations
import json
from pathlib import Path

def root_for(output: Path) -> Path:
    output = output.resolve()
    return output.parent / ".venom" / output.name

def read_json(output: Path, relative: str = "build.json"):
    path = root_for(output) / relative
    if not path.is_file():
        raise AssertionError(f"missing private Venom report: {path}")
    return json.loads(path.read_text(encoding="utf-8"))

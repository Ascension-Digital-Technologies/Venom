#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import sys
import tempfile
from pathlib import Path


def load_packager(root: Path):
    tool = root / "tools" / "package_release.py"
    sys.path.insert(0, str(tool.parent))
    spec = importlib.util.spec_from_file_location("venom_package_release", tool)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def files_under(path: Path) -> list[Path]:
    return [p for p in path.rglob("*") if p.is_file()]


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    packager = load_packager(root)

    with tempfile.TemporaryDirectory() as tmp:
        tmp_root = Path(tmp)

        windows_scripts = tmp_root / "windows" / "scripts"
        fmt = packager.copy_platform_scripts(root / "scripts", windows_scripts, "windows-x64")
        assert fmt == "bat"
        assert files_under(windows_scripts)
        assert all(p.suffix.lower() == ".bat" for p in files_under(windows_scripts))
        assert not list(windows_scripts.rglob("*.ps1"))
        assert not list(windows_scripts.rglob("*.sh"))
        build_bat = (windows_scripts / "build.bat").read_text(encoding="utf-8")
        assert "tools\\windows-scripts\\build.ps1" in build_bat
        assert (windows_scripts.parent / "tools" / "windows-scripts" / "build.ps1").is_file()

        linux_scripts = tmp_root / "linux" / "scripts"
        fmt = packager.copy_platform_scripts(root / "scripts", linux_scripts, "linux-x64")
        assert fmt == "sh"
        assert files_under(linux_scripts)
        assert all(p.suffix.lower() == ".sh" for p in files_under(linux_scripts))
        assert not list(linux_scripts.rglob("*.bat"))
        assert not list(linux_scripts.rglob("*.ps1"))

    print("release platform scripts smoke: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

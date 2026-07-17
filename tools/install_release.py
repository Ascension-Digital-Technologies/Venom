#!/usr/bin/env python3
"""Verify, install, inspect, or uninstall a Venom binary release."""
from __future__ import annotations

import argparse
import json
import os
import shutil
import stat
import subprocess
import sys
import tarfile
import tempfile
import zipfile
from pathlib import Path

SCHEMA_VERSION = 1
PRODUCT = "Venom - Secure Web Runtime"


def default_prefix() -> Path:
    if os.name == "nt":
        base = os.environ.get("LOCALAPPDATA") or str(Path.home() / "AppData" / "Local")
        return Path(base) / "Venom"
    return Path(os.environ.get("XDG_DATA_HOME", Path.home() / ".local" / "share")) / "venom"


def default_bin_dir() -> Path:
    if os.name == "nt":
        return default_prefix() / "bin"
    return Path.home() / ".local" / "bin"


def safe_member(name: str) -> bool:
    p = Path(name.replace("\\", "/"))
    return bool(name) and not p.is_absolute() and ".." not in p.parts


def extract_release(source: Path, destination: Path) -> Path:
    if source.is_dir():
        target = destination / source.name
        shutil.copytree(source, target, symlinks=False)
        return target
    lower = source.name.lower()
    if lower.endswith(".zip"):
        with zipfile.ZipFile(source) as zf:
            seen: set[str] = set()
            for info in zf.infolist():
                if not safe_member(info.filename):
                    raise SystemExit(f"unsafe release archive path: {info.filename}")
                normalized = Path(info.filename.replace("\\", "/")).as_posix()
                if normalized in seen:
                    raise SystemExit(f"duplicate release archive entry: {normalized}")
                seen.add(normalized)
            zf.extractall(destination)
    elif lower.endswith((".tar.gz", ".tgz")):
        with tarfile.open(source, "r:gz") as tf:
            seen: set[str] = set()
            for member in tf.getmembers():
                if not safe_member(member.name):
                    raise SystemExit(f"unsafe release archive path: {member.name}")
                normalized = Path(member.name.replace("\\", "/")).as_posix()
                if normalized in seen:
                    raise SystemExit(f"duplicate release archive entry: {normalized}")
                if member.issym() or member.islnk():
                    raise SystemExit(f"release archive links are not allowed: {member.name}")
                seen.add(normalized)
            tf.extractall(destination, filter="data")
    else:
        raise SystemExit("release source must be a directory, .zip, .tar.gz, or .tgz")

    roots = [p for p in destination.iterdir() if p.is_dir()]
    if len(roots) == 1:
        return roots[0]
    candidates = [p for p in destination.rglob("VERSION.txt") if p.parent.is_dir()]
    if len(candidates) == 1:
        return candidates[0].parent
    raise SystemExit("could not identify a single release root in the archive")


def parse_version(root: Path) -> str:
    version_file = root / "VERSION.txt"
    if not version_file.is_file():
        raise SystemExit("release is missing VERSION.txt")
    first = version_file.read_text(encoding="utf-8").splitlines()[0].strip()
    if not first.lower().startswith("venom "):
        raise SystemExit("release VERSION.txt has an invalid product version")
    version = first.split(None, 1)[1].strip()
    if not version or any(ch not in "0123456789.-+abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" for ch in version):
        raise SystemExit("release version contains unsupported characters")
    return version


def release_binary(root: Path) -> Path:
    names = ("venom.exe", "venom") if os.name == "nt" else ("venom", "venom.exe")
    for base in (root / "bin", root):
        for name in names:
            candidate = base / name
            if candidate.is_file():
                return candidate
    raise SystemExit("release is missing the Venom executable")


def verify_release(source: Path, verifier: Path, *, public_key: Path | None, require_signature: bool, openssl: str) -> None:
    if not verifier.is_file():
        raise SystemExit(f"release verifier not found: {verifier}")
    cmd = [sys.executable, str(verifier), str(source), "--require-supply-chain-metadata"]
    if require_signature:
        cmd.append("--strict-signature")
    if public_key:
        cmd.extend(["--public-key", str(public_key)])
    if openssl:
        cmd.extend(["--openssl", openssl])
    result = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
    if result.returncode != 0:
        sys.stdout.write(result.stdout)
        raise SystemExit("release verification failed; installation was not changed")


def set_executable(path: Path) -> None:
    if os.name != "nt":
        path.chmod(path.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)


def install(source: Path, prefix: Path, bin_dir: Path, verifier: Path, public_key: Path | None,
            require_signature: bool, openssl: str, force: bool) -> dict:
    source = source.resolve()
    verify_release(source, verifier.resolve(), public_key=public_key.resolve() if public_key else None,
                   require_signature=require_signature, openssl=openssl)
    prefix = prefix.expanduser().resolve()
    bin_dir = bin_dir.expanduser().resolve()
    releases = prefix / "releases"
    prefix.mkdir(parents=True, exist_ok=True)
    releases.mkdir(parents=True, exist_ok=True)
    bin_dir.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="venom-install-") as td:
        extracted = extract_release(source, Path(td))
        version = parse_version(extracted)
        binary = release_binary(extracted)
        final_root = releases / version
        if final_root.exists():
            if not force:
                raise SystemExit(f"Venom {version} is already installed; use --force to replace it")
            shutil.rmtree(final_root)
        stage = releases / f".{version}.installing"
        if stage.exists():
            shutil.rmtree(stage)
        shutil.copytree(extracted, stage, symlinks=False)
        os.replace(stage, final_root)

    installed_binary = final_root / binary.relative_to(extracted)
    command_path = bin_dir / ("venom.exe" if os.name == "nt" else "venom")
    temp_command = bin_dir / (command_path.name + ".tmp")
    shutil.copy2(installed_binary, temp_command)
    set_executable(temp_command)
    os.replace(temp_command, command_path)

    receipt = {
        "schema_version": SCHEMA_VERSION,
        "product": PRODUCT,
        "version": version,
        "release_root": str(final_root),
        "command": str(command_path),
        "source": str(source),
        "signature_required": require_signature,
    }
    receipt_path = prefix / "install.json"
    tmp_receipt = receipt_path.with_suffix(".json.tmp")
    tmp_receipt.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    os.replace(tmp_receipt, receipt_path)
    return receipt


def load_receipt(prefix: Path) -> tuple[Path, dict]:
    receipt_path = prefix.expanduser().resolve() / "install.json"
    if not receipt_path.is_file():
        raise SystemExit(f"no Venom installation receipt found at {receipt_path}")
    data = json.loads(receipt_path.read_text(encoding="utf-8"))
    if data.get("schema_version") != SCHEMA_VERSION or data.get("product") != PRODUCT:
        raise SystemExit("installation receipt is not recognized")
    return receipt_path, data


def uninstall(prefix: Path, keep_releases: bool) -> dict:
    receipt_path, receipt = load_receipt(prefix)
    prefix_resolved = prefix.expanduser().resolve()
    command = Path(receipt["command"]).expanduser().resolve()
    release_root = Path(receipt["release_root"]).expanduser().resolve()
    allowed_release_parent = (prefix_resolved / "releases").resolve()
    if release_root.parent != allowed_release_parent:
        raise SystemExit("refusing to remove release outside the installation prefix")
    if command.is_file():
        command.unlink()
    if not keep_releases and release_root.is_dir():
        shutil.rmtree(release_root)
    receipt_path.unlink()
    return {"removed_command": str(command), "removed_release": None if keep_releases else str(release_root)}


def status(prefix: Path) -> dict:
    _, receipt = load_receipt(prefix)
    command = Path(receipt["command"])
    release_root = Path(receipt["release_root"])
    receipt["command_exists"] = command.is_file()
    receipt["release_exists"] = release_root.is_dir()
    receipt["healthy"] = receipt["command_exists"] and receipt["release_exists"]
    return receipt


def output(data: dict, format_name: str) -> None:
    if format_name == "json":
        print(json.dumps(data, indent=2, sort_keys=True))
        return
    if "healthy" in data:
        print(f"Venom {data['version']} installation")
        print(f"  command: {data['command']} ({'present' if data['command_exists'] else 'missing'})")
        print(f"  release: {data['release_root']} ({'present' if data['release_exists'] else 'missing'})")
        print(f"  status: {'healthy' if data['healthy'] else 'repair required'}")
    elif data.get("verified"):
        print(f"Verified Venom release: {data['release']}")
    elif "version" in data:
        print(f"Installed Venom {data['version']}")
        print(f"  command: {data['command']}")
        print(f"  release: {data['release_root']}")
    else:
        print("Venom uninstalled")
        for key, value in data.items():
            if value:
                print(f"  {key.replace('_', ' ')}: {value}")


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest="command", required=True)

    install_ap = sub.add_parser("install", help="verify and install a binary release")
    install_ap.add_argument("release", type=Path)
    install_ap.add_argument("--prefix", type=Path, default=default_prefix())
    install_ap.add_argument("--bin-dir", type=Path, default=default_bin_dir())
    install_ap.add_argument("--verifier", type=Path, default=repo_root / "tools" / "verify_release.py")
    install_ap.add_argument("--public-key", type=Path)
    install_ap.add_argument("--require-signature", action="store_true")
    install_ap.add_argument("--openssl", default=os.environ.get("OPENSSL", "openssl"))
    install_ap.add_argument("--force", action="store_true")
    install_ap.add_argument("--format", choices=("text", "json"), default="text")

    verify_ap = sub.add_parser("verify", help="verify a release archive without installing it")
    verify_ap.add_argument("release", type=Path)
    verify_ap.add_argument("--verifier", type=Path, default=repo_root / "tools" / "verify_release.py")
    verify_ap.add_argument("--public-key", type=Path)
    verify_ap.add_argument("--require-signature", action="store_true")
    verify_ap.add_argument("--openssl", default=os.environ.get("OPENSSL", "openssl"))
    verify_ap.add_argument("--format", choices=("text", "json"), default="text")

    status_ap = sub.add_parser("status", help="inspect the recorded installation")
    status_ap.add_argument("--prefix", type=Path, default=default_prefix())
    status_ap.add_argument("--format", choices=("text", "json"), default="text")

    remove_ap = sub.add_parser("uninstall", help="remove the recorded installation")
    remove_ap.add_argument("--prefix", type=Path, default=default_prefix())
    remove_ap.add_argument("--keep-releases", action="store_true")
    remove_ap.add_argument("--format", choices=("text", "json"), default="text")

    args = ap.parse_args()
    if args.command == "install":
        data = install(args.release, args.prefix, args.bin_dir, args.verifier, args.public_key,
                       args.require_signature, args.openssl, args.force)
    elif args.command == "verify":
        verify_release(args.release.resolve(), args.verifier.resolve(), public_key=args.public_key.resolve() if args.public_key else None, require_signature=args.require_signature, openssl=args.openssl)
        data = {"verified": True, "release": str(args.release.resolve())}
    elif args.command == "status":
        data = status(args.prefix)
    else:
        data = uninstall(args.prefix, args.keep_releases)
    output(data, args.format)
    return 0 if data.get("healthy", True) else 60


if __name__ == "__main__":
    raise SystemExit(main())

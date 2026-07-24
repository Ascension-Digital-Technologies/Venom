#!/usr/bin/env python3
"""Build a clean Venom binary release folder and optional archive."""
from __future__ import annotations

import argparse
import hashlib
import io
import gzip
import os
import platform
import shutil
import stat
import subprocess
import sys
import tarfile
import zipfile
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable
from release_crypto import key_id_from_public_key, public_key_from_private, sign_ed25519, write_signature


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def dev_signature(manifest_bytes: bytes, key: str) -> str:
    h = hashlib.sha256()
    h.update(b'VENOM_DEV_RELEASE_SIGNATURE_V1\0')
    h.update(key.encode('utf-8'))
    h.update(b'\0')
    h.update(manifest_bytes)
    return h.hexdigest()


def sign_manifest_dev(out_dir: Path, key: str) -> Path:
    manifest = out_dir / 'RELEASE_MANIFEST.txt'
    sig = out_dir / 'RELEASE_MANIFEST.sig'
    digest = dev_signature(manifest.read_bytes(), key)
    sig.write_text(
        'VENOM_RELEASE_SIGNATURE_V1\n'
        'type=dev-sha256\n'
        'warning=development-only-not-for-production\n'
        f'digest={digest}\n',
        encoding='utf-8',
    )
    return sig


def sign_manifest_ed25519(out_dir: Path, private_key: Path, public_key: Path | None, key_id: str | None, openssl: str = 'openssl') -> tuple[Path, str, Path]:
    manifest = out_dir / 'RELEASE_MANIFEST.txt'
    sig = out_dir / 'RELEASE_MANIFEST.sig'
    if not private_key.is_file(): raise SystemExit(f'private key not found: {private_key}')
    if public_key is None: raise SystemExit('--sign ed25519 requires --public-key for post-package verification')
    pub = public_key
    resolved_key_id = key_id or key_id_from_public_key(pub, openssl)
    raw = manifest.read_bytes()
    write_signature(sig, key_id=resolved_key_id, signature=sign_ed25519(raw, private_key, openssl), manifest=raw)
    return sig, resolved_key_id, pub


def run_release_verify(repo_root: Path, target: Path, version: str, dev_key: str | None, public_key: Path | None, strict_signature: bool, openssl: str) -> None:
    verifier = repo_root / 'tools' / 'verify_release.py'
    if not verifier.exists():
        raise SystemExit(f'release verifier not found: {verifier}')
    cmd = [sys.executable, str(verifier), str(target), '--expect-version', version, '--require-supply-chain-metadata']
    if strict_signature:
        cmd.append('--strict-signature')
    if dev_key:
        cmd.extend(['--dev-insecure-key', dev_key])
    if public_key:
        cmd.extend(['--public-key', str(public_key)])
    if openssl:
        cmd.extend(['--openssl', openssl])
    result = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
    if result.returncode != 0:
        print(result.stdout)
        raise SystemExit(f'release verification failed for {target}')


def copy_tree(src: Path, dst: Path, ignore_names: set[str] | None = None) -> None:
    ignore_names = ignore_names or set()
    if not src.exists():
        return
    if dst.exists():
        shutil.rmtree(dst)
    def ignore(_dir: str, names: list[str]) -> set[str]:
        ignored = {n for n in names if n in ignore_names}
        ignored.update({n for n in names if n.startswith('.') and n not in {'.gitkeep'}})
        ignored.update({n for n in names if n in {'__pycache__', 'build', 'dist-release'}})
        return ignored
    shutil.copytree(src, dst, ignore=ignore)



def copy_platform_scripts(src: Path, dst: Path, target_triplet: str) -> str:
    """Copy the small platform-native launcher set into release/scripts."""
    if dst.exists():
        shutil.rmtree(dst)
    dst.mkdir(parents=True, exist_ok=True)
    is_windows = target_triplet.lower().startswith('windows-')
    platform_src = src / ('windows' if is_windows else 'linux')
    suffix = '.bat' if is_windows else '.sh'
    for path in sorted(platform_src.glob(f'*{suffix}')):
        shutil.copy2(path, dst / path.name)
        if not is_windows:
            make_executable(dst / path.name)
    if is_windows:
        copy_tree(src.parent / 'tools' / 'windows-scripts', dst.parent / 'tools' / 'windows-scripts')
    return 'bat' if is_windows else 'sh'

def parse_version(repo_root: Path) -> str:
    cmake = (repo_root / 'CMakeLists.txt').read_text(encoding='utf-8')
    for line in cmake.splitlines():
        line = line.strip()
        if line.startswith('VERSION '):
            return line.split(None, 1)[1].strip()
    return '0.0.0'


def candidate_executables(build_dir: Path, config: str) -> list[Path]:
    names = ['venom.exe', 'venom']
    subdirs = [Path('.'), Path(config), Path(config.capitalize()), Path('Release'), Path('Debug'), Path('RelWithDebInfo'), Path('MinSizeRel'), Path('bin'), Path(config) / 'bin']
    out: list[Path] = []
    for sub in subdirs:
        for name in names:
            out.append(build_dir / sub / name)
    return out


def find_venom(repo_root: Path, build_dir: Path, config: str, explicit: Path | None) -> Path:
    if explicit:
        path = explicit.resolve()
        if not path.exists():
            raise SystemExit(f'venom executable not found: {path}')
        return path
    for path in candidate_executables(build_dir.resolve(), config):
        if path.is_file():
            return path
    for path in candidate_executables(repo_root / 'build', config):
        if path.is_file():
            return path
    raise SystemExit(
        'could not find venom executable; pass --venom <path> or build first with scripts/linux/build.sh or scripts/windows/build.bat'
    )


def make_executable(path: Path) -> None:
    if os.name != 'nt':
        mode = path.stat().st_mode
        path.chmod(mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)


def write_manifest(out_dir: Path, version: str, repo_root: Path, release_sequence: int, release_channel: str, source_date_epoch: int) -> None:
    rows = []
    for path in sorted(out_dir.rglob('*')):
        if path.is_file() and path.name != 'RELEASE_MANIFEST.txt':
            rel = path.relative_to(out_dir).as_posix()
            rows.append((rel, path.stat().st_size, sha256(path)))
    lines = [
        'VENOM_RELEASE_MANIFEST_V1',
        f'version={version}',
        f'release_sequence={release_sequence}',
        f'release_channel={release_channel}',
        f'generated_utc={datetime.fromtimestamp(source_date_epoch, timezone.utc).replace(microsecond=0).isoformat()}',
        f'source_date_epoch={source_date_epoch}',
        f'host_platform={platform.platform()}',
        f'source_root={repo_root.name}',
        '',
        'files:',
    ]
    lines.extend(f'{digest}  {size:>10}  {rel}' for rel, size, digest in rows)
    (out_dir / 'RELEASE_MANIFEST.txt').write_text('\n'.join(lines) + '\n', encoding='utf-8')


def verify_release_binary(binary: Path) -> str:
    try:
        result = subprocess.run([str(binary), '--version'], check=True, text=True, capture_output=True, timeout=20)
    except Exception as exc:  # pragma: no cover - error path used by scripts
        raise SystemExit(f'release binary failed --version check: {exc}') from exc
    text = (result.stdout + result.stderr).strip()
    if 'venom ' not in text.lower():
        raise SystemExit(f'release binary returned unexpected version output: {text!r}')
    return text


def archive_release(out_dir: Path, version: str, archive_format: str, source_date_epoch: int, target_triplet: str) -> Path:
    suffix = '.zip' if archive_format == 'zip' else '.tar.gz'
    archive = out_dir.with_name(f'venom-v{version}-{target_triplet}{suffix}')
    if archive.exists():
        archive.unlink()
    root_name = out_dir.name
    dt = datetime.fromtimestamp(max(source_date_epoch, 315532800), timezone.utc)
    zip_dt = (dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second)
    if archive_format == 'zip':
        with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
            for path in sorted(out_dir.rglob('*')):
                if not path.is_file():
                    continue
                rel = (Path(root_name) / path.relative_to(out_dir)).as_posix()
                info = zipfile.ZipInfo(rel, date_time=zip_dt)
                info.compress_type = zipfile.ZIP_DEFLATED
                info.create_system = 3
                mode = 0o755 if os.access(path, os.X_OK) else 0o644
                info.external_attr = (mode & 0xFFFF) << 16
                zf.writestr(info, path.read_bytes(), compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
    else:
        with archive.open('wb') as raw, gzip.GzipFile(filename='', mode='wb', fileobj=raw, mtime=source_date_epoch, compresslevel=9) as gz, tarfile.open(fileobj=gz, mode='w') as tf:
            for path in sorted(out_dir.rglob('*')):
                if not path.is_file():
                    continue
                rel = (Path(root_name) / path.relative_to(out_dir)).as_posix()
                data = path.read_bytes()
                info = tarfile.TarInfo(rel)
                info.size = len(data)
                info.mtime = source_date_epoch
                info.uid = info.gid = 0
                info.uname = info.gname = ''
                info.mode = 0o755 if os.access(path, os.X_OK) else 0o644
                tf.addfile(info, io.BytesIO(data))
    return archive


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument('--build-dir', type=Path, default=None)
    ap.add_argument('--out', type=Path, default=None)
    ap.add_argument('--venom', type=Path, default=None, help='explicit compiled venom executable')
    ap.add_argument('--config', default='Release')
    ap.add_argument('--target-triplet', default=None, help='archive target name such as windows-x64 or linux-arm64')
    ap.add_argument('--archive', choices=['none', 'zip', 'tar.gz'], default='none')
    ap.add_argument('--sign', choices=['none', 'dev-sha256', 'ed25519'], default='none')
    ap.add_argument('--dev-insecure-key', default=None, help='development-only signature key for portable smoke tests')
    ap.add_argument('--private-key', type=Path, default=None, help='offline Ed25519 private key PEM')
    ap.add_argument('--key-id', default=None, help='explicit trusted key identifier; default is SHA-256 fingerprint')
    ap.add_argument('--release-sequence', type=int, default=90, help='monotonic anti-rollback release sequence')
    ap.add_argument('--release-channel', choices=['development','preview','stable'], default='stable')
    ap.add_argument('--public-key', type=Path, default=None, help='optional public key used for post-package self verification')
    ap.add_argument('--openssl', default=os.environ.get('OPENSSL', 'openssl'))
    ap.add_argument('--skip-verify', action='store_true', help='do not run final release verifier after packaging')
    ap.add_argument('--source-date-epoch', type=int, default=None, help='reproducible build timestamp; defaults to SOURCE_DATE_EPOCH or 0')
    ap.add_argument('--no-supply-chain-metadata', action='store_true', help='omit SBOM/provenance metadata (not allowed for stable releases)')
    args = ap.parse_args()

    repo_root = args.repo_root.resolve()
    version = parse_version(repo_root)
    machine = platform.machine().lower().replace('amd64','x64').replace('x86_64','x64').replace('aarch64','arm64')
    system = platform.system().lower().replace('darwin','macos')
    target_triplet = args.target_triplet or f'{system}-{machine}'
    build_dir = (args.build_dir or (repo_root / 'build')).resolve()
    out_dir = (args.out or (repo_root / 'dist-release')).resolve()
    venom = find_venom(repo_root, build_dir, args.config, args.venom)
    source_date_epoch = args.source_date_epoch
    if source_date_epoch is None:
        raw_epoch = os.environ.get('SOURCE_DATE_EPOCH', '0')
        try: source_date_epoch = int(raw_epoch)
        except ValueError: raise SystemExit('SOURCE_DATE_EPOCH must be a non-negative integer')
    if source_date_epoch < 0: raise SystemExit('--source-date-epoch must be non-negative')
    if args.release_channel == 'stable' and args.no_supply_chain_metadata:
        raise SystemExit('stable releases require SBOM and provenance metadata')

    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True)

    bin_dir = out_dir / 'bin'
    bin_dir.mkdir(parents=True, exist_ok=True)
    release_binary = bin_dir / ('venom.exe' if venom.suffix.lower() == '.exe' or os.name == 'nt' else 'venom')
    shutil.copy2(venom, release_binary)
    make_executable(release_binary)
    helper_name = 'venom_hardener_worker.exe' if release_binary.suffix.lower() == '.exe' else 'venom_hardener_worker'
    helper_source = venom.parent / helper_name
    if not helper_source.is_file():
        raise SystemExit(f'hardener worker executable not found beside Venom: {helper_source}')
    helper_binary = bin_dir / helper_name
    shutil.copy2(helper_source, helper_binary)
    make_executable(helper_binary)
    version_text = verify_release_binary(release_binary)

    copy_tree(repo_root / 'docs', out_dir / 'docs')
    copy_tree(repo_root / 'examples', out_dir / 'examples')
    copy_tree(repo_root / 'packages', out_dir / 'packages')
    copy_tree(repo_root / 'integrations', out_dir / 'integrations')
    copy_tree(repo_root / 'contracts', out_dir / 'contracts')
    release_script_format = copy_platform_scripts(repo_root / 'scripts', out_dir / 'scripts', target_triplet)
    tools_out = out_dir / 'tools'
    tools_out.mkdir(parents=True, exist_ok=True)
    for tool_name in ('verify_release.py', 'release_crypto.py', 'sign_release.py', 'turbojs_wasm_preflight.py', 'turbojs_wasm_cutover.py', 'turbojs_runtime_lifecycle.py', 'setup_emscripten.py', 'build_emscripten.py', 'analyze_dist.py', 'wasm_exports.py', 'embed_wasm.py', 'generate_release_metadata.py', 'verify_release_set.py', 'install_release.py', 'public_release_gate.py', 'generate_contract_manifest.py', 'check_contract_upgrade.py', 'generate_release_metadata.py'):
        tool_src = repo_root / 'tools' / tool_name
        if tool_src.exists():
            shutil.copy2(tool_src, tools_out / tool_name)
            make_executable(tools_out / tool_name)

    licenses = out_dir / 'licenses'
    licenses.mkdir(parents=True, exist_ok=True)
    turbojs_license = repo_root / 'third_party' / 'turbojs' / 'LICENSE'
    if turbojs_license.exists():
        shutil.copy2(turbojs_license, licenses / 'TURBOJS-LICENSE')
    root_license = repo_root / 'LICENSE'
    if root_license.exists():
        shutil.copy2(root_license, licenses / 'VENOM-LICENSE')

    for rel in ('README.md', 'CHANGES.md', 'SECURITY.md', 'SUPPORT.md', 'NOTICE.md', 'toolchains.lock.json'):
        src = repo_root / rel
        if src.exists():
            shutil.copy2(src, out_dir / rel)

    (out_dir / 'VERSION.txt').write_text(
        f'venom {version}\nverified_binary_output={version_text}\nrelease_script_format={release_script_format}\n', encoding='utf-8'
    )
    runtime_dir = out_dir / 'runtime'
    runtime_dir.mkdir(parents=True, exist_ok=True)
    embedded_runtime = repo_root / 'build' / 'turbojs-wasm' / 'turbojs-runtime.wasm'
    if embedded_runtime.is_file():
        shutil.copy2(embedded_runtime, runtime_dir / 'turbojs-runtime.wasm')
    contract_tool = repo_root / 'tools' / 'generate_contract_manifest.py'
    subprocess.run([sys.executable, str(contract_tool), '--header', str(repo_root / 'src' / 'contracts' / 'product_contracts.hpp'), '--version', version, '--out', str(out_dir / 'CONTRACTS.json')], check=True)
    for src_name, dst_name in (('release_install.ps1','install.ps1'),('release_install.sh','install.sh')):
        src = repo_root / 'tools' / src_name
        if src.is_file():
            shutil.copy2(src, out_dir / dst_name)
            make_executable(out_dir / dst_name)
    (out_dir / 'uninstall.ps1').write_text("$Root=Split-Path -Parent $MyInvocation.MyCommand.Path\npython (Join-Path $Root 'tools\\install_release.py') uninstall @args\nexit $LASTEXITCODE\n", encoding='utf-8')
    (out_dir / 'uninstall.sh').write_text('#!/usr/bin/env sh\nset -eu\nROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\nexec python3 "$ROOT/tools/install_release.py" uninstall "$@"\n', encoding='utf-8')
    make_executable(out_dir / 'uninstall.sh')
    if args.release_sequence < 0: raise SystemExit('--release-sequence must be non-negative')
    if not args.no_supply_chain_metadata:
        metadata_tool = repo_root / 'tools' / 'generate_release_metadata.py'
        cmd = [sys.executable, str(metadata_tool), '--repo-root', str(repo_root), '--release-root', str(out_dir), '--version', version, '--source-date-epoch', str(source_date_epoch), '--release-sequence', str(args.release_sequence), '--release-channel', args.release_channel, '--target-triplet', target_triplet]
        subprocess.run(cmd, check=True, timeout=60)
    write_manifest(out_dir, version, repo_root, args.release_sequence, args.release_channel, source_date_epoch)

    signature_path = None
    strict_signature = False
    verify_dev_key = None
    if args.sign == 'dev-sha256':
        if not args.dev_insecure_key:
            raise SystemExit('--sign dev-sha256 requires --dev-insecure-key')
        signature_path = sign_manifest_dev(out_dir, args.dev_insecure_key)
        strict_signature = True
        verify_dev_key = args.dev_insecure_key
    elif args.sign == 'ed25519':
        if not args.private_key or not args.public_key: raise SystemExit('--sign ed25519 requires --private-key and --public-key')
        signature_path, resolved_key_id, signing_public_key = sign_manifest_ed25519(out_dir, args.private_key.resolve(), args.public_key.resolve() if args.public_key else None, args.key_id, args.openssl)
        strict_signature = True


    if not args.skip_verify:
        run_release_verify(repo_root, out_dir, version, verify_dev_key, args.public_key.resolve() if args.public_key else None, strict_signature, args.openssl)

    archive_path = None
    if args.archive != 'none':
        archive_path = archive_release(out_dir, version, args.archive, source_date_epoch, target_triplet)
        if not args.skip_verify:
            run_release_verify(repo_root, archive_path, version, verify_dev_key, args.public_key.resolve() if args.public_key else None, strict_signature, args.openssl)

    print(f'release directory: {out_dir}')
    print(f'release binary: {release_binary}')
    print(f'version: {version_text}')
    if signature_path:
        print(f'signature: {signature_path}')
    if archive_path:
        print(f'archive: {archive_path}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

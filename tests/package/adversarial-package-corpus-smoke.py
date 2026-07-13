#!/usr/bin/env python3
from __future__ import annotations

import shutil
import struct
import subprocess
import sys
from pathlib import Path


def find_package(dist: Path) -> Path:
    candidates = sorted((dist / 'assets' / 'app').glob('app.*.vbc'))
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise SystemExit('missing app package under assets/app/app.<hash>.vbc')

HEADER_SIZE = 80
ENTRY_SIZE = 48
HASH_OFFSET = 72


def fnv1a64(data: bytes) -> int:
    value = 0xCBF29CE484222325
    for byte in data:
        value ^= byte
        value = (value * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return value


def rehash(data: bytearray) -> bytes:
    data[HASH_OFFSET:HASH_OFFSET + 8] = b"\0" * 8
    struct.pack_into("<Q", data, HASH_OFFSET, fnv1a64(data))
    return bytes(data)


def run(cmd: list[str], *, timeout: int = 20, expect_ok: bool = True) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
    if expect_ok and result.returncode != 0:
        raise RuntimeError(f"command failed ({result.returncode}): {' '.join(cmd)}\n{result.stdout}")
    return result


def main() -> int:
    if len(sys.argv) != 5:
        print("usage: adversarial-package-corpus-smoke.py <venom> <site> <work> <corpus>", file=sys.stderr)
        return 2
    venom, site, work, corpus = map(Path, sys.argv[1:])
    shutil.rmtree(work, ignore_errors=True)
    work.mkdir(parents=True)
    corpus.mkdir(parents=True, exist_ok=True)
    dist = work / "dist"
    remote_cache = Path(__file__).resolve().parents[1] / "fixtures" / "remote-cache"
    run([str(venom), "build", str(site), "--out", str(dist), "--profile", "debug", "--vendor-cache", str(remote_cache), "--offline"])
    package = find_package(dist)
    original = bytearray(package.read_bytes())
    if len(original) < HEADER_SIZE + 2 * ENTRY_SIZE:
        raise RuntimeError("fixture package is unexpectedly small")

    section_count = struct.unpack_from("<I", original, 20)[0]
    if section_count < 2:
        raise RuntimeError("fixture package needs at least two sections")
    entry0, entry1 = HEADER_SIZE, HEADER_SIZE + ENTRY_SIZE

    cases: dict[str, bytes] = {
        "empty.vbc": b"",
        "one-byte.vbc": b"V",
        "truncated-header.vbc": bytes(original[:79]),
        "truncated-table.vbc": bytes(original[:HEADER_SIZE + ENTRY_SIZE - 1]),
    }

    bad = bytearray(original); bad[0] ^= 0xFF; cases["bad-magic.vbc"] = bytes(bad)
    bad = bytearray(original); bad[-1] ^= 0x80; cases["bad-package-hash.vbc"] = bytes(bad)
    bad = bytearray(original); struct.pack_into("<I", bad, 20, 0xFFFFFFFF); cases["section-count-overflow.vbc"] = rehash(bad)
    bad = bytearray(original); struct.pack_into("<Q", bad, 24, 0xFFFFFFFFFFFFFFF0); cases["table-offset-wrap.vbc"] = rehash(bad)
    bad = bytearray(original); struct.pack_into("<Q", bad, 48, 2 * 1024 * 1024); cases["name-table-limit.vbc"] = rehash(bad)
    bad = bytearray(original); struct.pack_into("<I", bad, entry0, 0xFFFFFFFF); cases["unknown-section-type.vbc"] = rehash(bad)
    bad = bytearray(original); struct.pack_into("<I", bad, entry0 + 4, 0x80000000); cases["unknown-section-flags.vbc"] = rehash(bad)
    bad = bytearray(original); struct.pack_into("<Q", bad, entry0 + 32, 33 * 1024 * 1024); cases["decoded-section-limit.vbc"] = rehash(bad)

    bad = bytearray(original)
    bad[entry1:entry1 + 16] = bad[entry0:entry0 + 16]
    cases["duplicate-section-identity.vbc"] = rehash(bad)

    bad = bytearray(original)
    first_offset = struct.unpack_from("<Q", bad, entry0 + 16)[0]
    struct.pack_into("<Q", bad, entry1 + 16, first_offset)
    cases["overlapping-payload.vbc"] = rehash(bad)

    for name, payload in cases.items():
        target = corpus / name
        target.write_bytes(payload)
        try:
            result = run([str(venom), "inspect", str(target)], timeout=3, expect_ok=False)
        except subprocess.TimeoutExpired as exc:
            raise RuntimeError(f"parser hung on {name}") from exc
        if result.returncode == 0:
            raise RuntimeError(f"malformed corpus item was accepted: {name}\n{result.stdout}")

    valid = run([str(venom), "inspect", str(package)], timeout=5, expect_ok=False)
    if valid.returncode != 0:
        raise RuntimeError(f"valid package was rejected after hardening\n{valid.stdout}")
    print(f"v1.0.1 adversarial package corpus: PASS ({len(cases)} malformed cases)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

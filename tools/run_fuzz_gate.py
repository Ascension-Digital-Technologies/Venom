#!/usr/bin/env python3
"""Deterministic hostile-input gate for Venom fuzz/replay targets."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess
import sys
import tempfile
import time


def mutate(data: bytes, seed: int, count: int) -> list[bytes]:
    rng = random.Random(seed)
    out: list[bytes] = []
    if not data:
        data = b"\x00"
    for i in range(count):
        buf = bytearray(data)
        mode = i % 6
        if mode == 0:
            pos = rng.randrange(len(buf)); buf[pos] ^= 1 << rng.randrange(8)
        elif mode == 1:
            pos = rng.randrange(len(buf)); buf[pos] = rng.randrange(256)
        elif mode == 2:
            cut = rng.randrange(len(buf) + 1); del buf[cut:]
        elif mode == 3:
            pos = rng.randrange(len(buf) + 1); buf[pos:pos] = bytes(rng.randrange(256) for _ in range(min(16, 1 + rng.randrange(16))))
        elif mode == 4:
            pos = rng.randrange(len(buf)); span = min(len(buf) - pos, 1 + rng.randrange(16)); buf[pos:pos+span] = b"\xff" * span
        else:
            pos = rng.randrange(len(buf)); span = min(len(buf) - pos, 1 + rng.randrange(16)); buf[pos:pos+span] = b"\x00" * span
        out.append(bytes(buf))
    return out


def run_one(exe: Path, path: Path, timeout: float) -> tuple[bool, str, float]:
    started = time.monotonic()
    try:
        proc = subprocess.run([str(exe), str(path)], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              timeout=timeout, check=False)
        elapsed = time.monotonic() - started
        ok = proc.returncode == 0
        detail = (proc.stderr or proc.stdout).decode("utf-8", "replace")[-500:]
        return ok, detail, elapsed
    except subprocess.TimeoutExpired:
        return False, "timeout", time.monotonic() - started


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--replay", required=True, type=Path)
    ap.add_argument("--corpus", required=True, type=Path)
    ap.add_argument("--mutations-per-seed", type=int, default=32)
    ap.add_argument("--seed", type=int, default=0x56454E4F)
    ap.add_argument("--timeout", type=float, default=3.0)
    ap.add_argument("--json-out", type=Path)
    ns = ap.parse_args()
    if not ns.replay.is_file():
        ap.error(f"replay executable not found: {ns.replay}")
    seeds = sorted(p for p in ns.corpus.rglob("*") if p.is_file())
    if not seeds:
        ap.error(f"empty corpus: {ns.corpus}")

    failures: list[dict[str, object]] = []
    total = 0
    max_seconds = 0.0
    with tempfile.TemporaryDirectory(prefix="venom-fuzz-gate-") as td:
        root = Path(td)
        for seed_index, seed_path in enumerate(seeds):
            original = seed_path.read_bytes()
            cases = [original, *mutate(original, ns.seed ^ seed_index, ns.mutations_per_seed)]
            for case_index, payload in enumerate(cases):
                digest = hashlib.sha256(payload).hexdigest()
                case_path = root / digest
                case_path.write_bytes(payload)
                ok, detail, elapsed = run_one(ns.replay, case_path, ns.timeout)
                total += 1
                max_seconds = max(max_seconds, elapsed)
                if not ok:
                    failures.append({"seed": str(seed_path), "case": case_index,
                                     "sha256": digest, "detail": detail, "seconds": elapsed})

    result = {
        "schema": "venom-fuzz-gate-v1",
        "replay": str(ns.replay),
        "corpus": str(ns.corpus),
        "seed_files": len(seeds),
        "cases": total,
        "failures": failures,
        "max_case_seconds": max_seconds,
        "passed": not failures,
    }
    text = json.dumps(result, indent=2, sort_keys=True)
    if ns.json_out:
        ns.json_out.parent.mkdir(parents=True, exist_ok=True)
        ns.json_out.write_text(text + "\n", encoding="utf-8")
    print(text)
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

import base64
import hashlib
import os
import shutil
import subprocess
import sys
from pathlib import Path


def run(command: list[str], *, env: dict[str, str] | None = None, ok: bool = True) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env, timeout=90)
    if ok and result.returncode != 0:
        raise RuntimeError(f"command failed ({result.returncode}): {' '.join(command)}\n{result.stdout}")
    if not ok and result.returncode == 0:
        raise RuntimeError(f"command unexpectedly succeeded: {' '.join(command)}\n{result.stdout}")
    return result


def write_site(root: Path, url: str, integrity: str = "") -> None:
    root.mkdir(parents=True, exist_ok=True)
    integrity_attr = f' integrity="{integrity}"' if integrity else ""
    (root / "index.html").write_text(
        f'<!doctype html><html><body><main id="app">vendor</main><script src="{url}"{integrity_attr}></script></body></html>',
        encoding="utf-8",
    )



def write_lock(root: Path, url: str, data: bytes, integrity_status: str) -> None:
    digest = hashlib.sha256(data).hexdigest()
    (root / "venom.lock").write_text(
        "VENOM_VENDOR_LOCK_V1\n"
        "version=1\n"
        "entry_count=1\n"
        "url\tsha256\tbytes\tintegrity\n"
        f"{url}\t{digest}\t{len(data)}\t{integrity_status}\n",
        encoding="utf-8",
    )

def expected_body() -> bytes:
    return b"globalThis.__venomFetchedVendor=(globalThis.__venomFetchedVendor||0)+1;\n"


def sri(data: bytes, algorithm: str) -> str:
    digest = getattr(hashlib, algorithm)(data).digest()
    return f"{algorithm}-" + base64.b64encode(digest).decode("ascii")


def main() -> int:
    if len(sys.argv) != 4:
        print("usage: remote-vendor-smoke.py <venom> <fake-curl> <work>", file=sys.stderr)
        return 2
    venom = Path(sys.argv[1]).resolve()
    fake_curl = Path(sys.argv[2]).resolve()
    work = Path(sys.argv[3]).resolve()
    shutil.rmtree(work, ignore_errors=True)
    work.mkdir(parents=True)
    env = os.environ.copy()
    env["VENOM_CURL"] = str(fake_curl)
    version_result = run([str(venom), "--version"])
    version = version_result.stdout.strip().split()[-1]
    env["VENOM_FAKE_CURL_EXPECT_USER_AGENT"] = f"Venom/{version} remote-vendor"

    url = "https://cdn.example.test/vendor.js"
    site = work / "site"
    cache = work / "cache"
    dist = work / "dist"
    write_site(site, url, sri(expected_body(), "sha256"))

    run([str(venom), "build", str(site), "--out", str(dist), "--vendor-cache", str(cache), "--refresh-vendors"], env=env)
    verify = run([str(venom), "verify-runtime", str(dist), "--target", "browser", "--require-real-engine"])
    for marker in (
        "remote_vendor_metadata: yes",
        "remote_vendor_count: 1",
        "vendored_remote_chunks: 1",
        "runtime_remote_chunks: 0",
        "release_status: PASS",
    ):
        if marker not in verify.stdout:
            raise RuntimeError(f"missing remote vendor marker {marker!r}\n{verify.stdout}")

    key = hashlib.sha256(url.encode()).hexdigest()
    body_path = cache / f"{key}.js"
    metadata_path = cache / f"{key}.meta"
    if body_path.read_bytes() != expected_body() or not metadata_path.is_file():
        raise RuntimeError("remote vendor cache was not written deterministically")
    lock_path = site / "venom.lock"
    lock_text = lock_path.read_text(encoding="utf-8")
    if "VENOM_VENDOR_LOCK_V1" not in lock_text or hashlib.sha256(expected_body()).hexdigest() not in lock_text or "verified-sha256" not in lock_text:
        raise RuntimeError(f"venom.lock was not generated correctly\n{lock_text}")

    offline_dist = work / "offline-dist"
    run([str(venom), "build", str(site), "--out", str(offline_dist), "--vendor-cache", str(cache), "--offline"])

    original = body_path.read_bytes()
    body_path.write_bytes(original + b"//tampered\n")
    corrupt = run([str(venom), "build", str(site), "--out", str(work / "corrupt-dist"), "--vendor-cache", str(cache), "--offline"], ok=False)
    if "cache miss in offline mode" not in corrupt.stdout:
        raise RuntimeError(f"corrupt vendor cache was not rejected fail-closed\n{corrupt.stdout}")
    body_path.write_bytes(original)

    missing = run([str(venom), "build", str(site), "--out", str(work / "missing-dist"), "--vendor-cache", str(work / "missing-cache"), "--offline"], ok=False)
    if "cache miss in offline mode" not in missing.stdout:
        raise RuntimeError(f"offline vendor cache miss was not rejected\n{missing.stdout}")


    for algorithm in ("sha384", "sha512"):
        sri_site = work / f"{algorithm}-site"
        write_site(sri_site, url, sri(expected_body(), algorithm))
        write_lock(sri_site, url, expected_body(), f"verified-{algorithm}")
        sri_dist = work / f"{algorithm}-dist"
        run([str(venom), "build", str(sri_site), "--out", str(sri_dist), "--vendor-cache", str(cache), "--offline"])
        sri_verify = run([str(venom), "verify-runtime", str(sri_dist), "--target", "browser", "--require-real-engine"])
        if "release_status: PASS" not in sri_verify.stdout:
            raise RuntimeError(f"{algorithm} SRI build did not verify\n{sri_verify.stdout}")

    bad_sri_site = work / "bad-sri-site"
    write_site(bad_sri_site, url, "sha256-AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=")
    bad_sri = run([str(venom), "build", str(bad_sri_site), "--out", str(work / "bad-sri-dist"), "--vendor-cache", str(cache), "--offline"], ok=False)
    if "integrity mismatch" not in bad_sri.stdout:
        raise RuntimeError(f"SRI mismatch was not rejected\n{bad_sri.stdout}")

    for algorithm, invalid in (
        ("sha384", "sha384-" + "A" * 64),
        ("sha512", "sha512-" + "A" * 88),
    ):
        bad_site = work / f"bad-{algorithm}-site"
        write_site(bad_site, url, invalid)
        bad_result = run([str(venom), "build", str(bad_site), "--out", str(work / f"bad-{algorithm}-dist"), "--vendor-cache", str(cache), "--offline"], ok=False)
        if "SRI integrity mismatch" not in bad_result.stdout:
            raise RuntimeError(f"{algorithm} SRI mismatch was not rejected\n{bad_result.stdout}")

    unsupported_site = work / "unsupported-sri-site"
    write_site(unsupported_site, url, "md5-AAAAAAAAAAAAAAAAAAAAAA==")
    unsupported = run([str(venom), "build", str(unsupported_site), "--out", str(work / "unsupported-sri-dist"), "--vendor-cache", str(cache), "--offline"], ok=False)
    if "no supported SRI algorithm" not in unsupported.stdout:
        raise RuntimeError(f"unsupported SRI algorithm was not rejected\n{unsupported.stdout}")

    # A modified lock must fail closed before a production artifact can be accepted.
    good_lock = lock_path.read_text(encoding="utf-8")
    lock_path.write_text(good_lock.replace(hashlib.sha256(expected_body()).hexdigest(), "0" * 64), encoding="utf-8")
    bad_lock = run([str(venom), "build", str(site), "--out", str(work / "bad-lock-dist"), "--vendor-cache", str(cache), "--offline"], ok=False)
    if "venom.lock rejected changed remote dependency" not in bad_lock.stdout:
        raise RuntimeError(f"tampered venom.lock was not rejected\n{bad_lock.stdout}")
    lock_path.write_text(good_lock, encoding="utf-8")

    # A dependency without SRI is still pinned by venom.lock. If its cached body
    # is unavailable and the network returns changed bytes, the lock rejects them
    # before the new response can replace the reviewed cache entry.
    unlocked_url = "https://cdn.example.test/unlocked.js"
    unlocked_site = work / "unlocked-site"
    unlocked_cache = work / "unlocked-cache"
    write_site(unlocked_site, unlocked_url)
    run([str(venom), "build", str(unlocked_site), "--out", str(work / "unlocked-dist"), "--vendor-cache", str(unlocked_cache), "--refresh-vendors"], env=env)
    unlocked_key = hashlib.sha256(unlocked_url.encode()).hexdigest()
    unlocked_body_path = unlocked_cache / f"{unlocked_key}.js"
    unlocked_metadata_path = unlocked_cache / f"{unlocked_key}.meta"
    unlocked_lock_path = unlocked_site / "venom.lock"
    original_unlocked_lock = unlocked_lock_path.read_text(encoding="utf-8")
    unlocked_body_path.unlink()

    changed_env = env.copy()
    changed_env["VENOM_FAKE_CURL_VARIANT"] = "changed"
    changed = run([str(venom), "build", str(unlocked_site), "--out", str(work / "changed-dist"), "--vendor-cache", str(unlocked_cache)], env=changed_env, ok=False)
    if "venom.lock rejected changed remote dependency" not in changed.stdout:
        raise RuntimeError(f"changed dependency was not rejected by venom.lock\n{changed.stdout}")
    if unlocked_body_path.exists():
        raise RuntimeError("unreviewed changed dependency overwrote the pinned cache body")

    # An explicit refresh is the only path that can accept and re-pin reviewed bytes.
    run([str(venom), "build", str(unlocked_site), "--out", str(work / "refreshed-dist"), "--vendor-cache", str(unlocked_cache), "--refresh-vendors"], env=changed_env)
    refreshed_body = unlocked_body_path.read_bytes()
    refreshed_digest = hashlib.sha256(refreshed_body).hexdigest()
    refreshed_lock = unlocked_lock_path.read_text(encoding="utf-8")
    if refreshed_digest not in refreshed_lock or refreshed_lock == original_unlocked_lock:
        raise RuntimeError("--refresh-vendors did not update venom.lock to reviewed bytes")

    for name, denied_url, expected in (
        ("http", "http://cdn.example.test/vendor.js", "requires HTTPS"),
        ("private", "https://127.0.0.1/vendor.js", "private or local host"),
        ("credentials", "https://user:pass@cdn.example.test/vendor.js", "credentials are denied"),
    ):
        denied_site = work / f"{name}-site"
        write_site(denied_site, denied_url)
        denied = run([str(venom), "build", str(denied_site), "--out", str(work / f"{name}-dist"), "--vendor-cache", str(cache), "--refresh-vendors"], env=env, ok=False)
        if expected not in denied.stdout:
            raise RuntimeError(f"{name} vendor URL was not rejected correctly\n{denied.stdout}")

    # A downloader that exits successfully without honoring --output must fail
    # with a precise error and leave no temporary artifacts behind. This covers
    # the Windows regression where curl streamed a response to the console and
    # the compiler later reported a misleading file-read failure.
    no_output_site = work / "no-output-site"
    no_output_cache = work / "no-output cache"
    write_site(no_output_site, "https://cdn.example.test/no-output.js")
    no_output_env = env.copy()
    no_output_env["VENOM_FAKE_CURL_NO_OUTPUT"] = "1"
    no_output = run(
        [str(venom), "build", str(no_output_site), "--out", str(work / "no-output-dist"),
         "--vendor-cache", str(no_output_cache), "--refresh-vendors"],
        env=no_output_env,
        ok=False,
    )
    if "completed without producing an output file" not in no_output.stdout:
        raise RuntimeError(f"missing-output downloader failure was not diagnosed correctly\n{no_output.stdout}")
    if list(no_output_cache.glob("*.download")) or list(no_output_cache.glob("*.stderr")):
        raise RuntimeError("missing-output downloader left temporary files behind")

    failure_site = work / "failure-site"
    failure_cache = work / "failure-cache"
    write_site(failure_site, "https://cdn.example.test/download-failure.js")
    failure = run(
        [str(venom), "build", str(failure_site), "--out", str(work / "failure-dist"),
         "--vendor-cache", str(failure_cache), "--refresh-vendors"],
        env=env,
        ok=False,
    )
    if "curl exit 22" not in failure.stdout or "simulated curl download failure" not in failure.stdout:
        raise RuntimeError(f"curl failure detail was not preserved\n{failure.stdout}")
    if list(failure_cache.glob("*.download")) or list(failure_cache.glob("*.stderr")):
        raise RuntimeError("failed downloader left temporary files behind")

    html_site = work / "html-site"
    write_site(html_site, "https://cdn.example.test/html-response.js")
    html_result = run([str(venom), "build", str(html_site), "--out", str(work / "html-dist"), "--vendor-cache", str(work / "html-cache"), "--refresh-vendors"], env=env, ok=False)
    if "returned HTML instead of JavaScript" not in html_result.stdout:
        raise RuntimeError(f"HTML vendor response was not rejected\n{html_result.stdout}")

    print("remote vendor smoke: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

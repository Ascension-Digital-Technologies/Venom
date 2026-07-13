#!/usr/bin/env python3
import pathlib
import subprocess
import sys

if len(sys.argv) != 5:
    print("usage: production-release-gates-smoke.py <venom> <site> <out-root> <key-file>")
    raise SystemExit(2)

venom = pathlib.Path(sys.argv[1])
site = pathlib.Path(sys.argv[2])
out_root = pathlib.Path(sys.argv[3])
key_file = pathlib.Path(sys.argv[4])

subprocess.run([str(venom), "keygen", "--out", str(key_file), "--force"], check=True)
key_text = key_file.read_text().strip()
if len(key_text) != 64 or any(ch not in "0123456789abcdefABCDEF" for ch in key_text):
    raise SystemExit("keygen did not write a 64-character hex package key")

browser_out = out_root / "browser"
subprocess.run([str(venom), "build", str(site), "--out", str(browser_out), "--profile", "browser-protect"], check=True)
browser_check = subprocess.run([str(venom), "release-check", str(browser_out), "--target", "browser"], check=True, text=True, stdout=subprocess.PIPE)
if "release_status: PASS" not in browser_check.stdout:
    raise SystemExit("browser-protect release-check did not pass")
if "provider_runtime_sections: 0" in browser_check.stdout:
    raise SystemExit("browser-protect did not use runtime-decodable protected sections")
if (browser_out / "assets" / "asset-manifest.txt").exists():
    raise SystemExit("browser-protect emitted external asset-manifest.txt")

audited_fail = subprocess.run([str(venom), "release-check", str(browser_out), "--target", "native", "--require-audited-crypto"], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
if audited_fail.returncode == 0 or "release_status: FAIL" not in audited_fail.stdout:
    raise SystemExit("release-check should fail when audited crypto is required for a browser-protect package")

native_out = out_root / "native"
native_build = subprocess.run([str(venom), "build", str(site), "--out", str(native_out), "--profile", "native-protect", "--key-file", str(key_file), "--require-audited-crypto"], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
if native_build.returncode != 0:
    if "libsodium provider is not available" in native_build.stdout:
        print("libsodium unavailable; native-protect release gate path skipped")
        raise SystemExit(0)
    print(native_build.stdout)
    raise SystemExit(native_build.returncode)
if "provider_libsodium_sections: 0" in native_build.stdout or "native_private_crypto: yes" not in native_build.stdout:
    raise SystemExit("native-protect build did not report libsodium private crypto")

native_check = subprocess.run([str(venom), "release-check", str(native_out), "--target", "native", "--key-file", str(key_file), "--require-audited-crypto"], check=True, text=True, stdout=subprocess.PIPE)
if "release_status: PASS" not in native_check.stdout:
    raise SystemExit("native-protect release-check did not pass")
if "provider_libsodium_sections: 0" in native_check.stdout:
    raise SystemExit("native-protect release-check did not detect libsodium sections")

missing_key_check = subprocess.run([str(venom), "release-check", str(native_out), "--target", "native", "--require-audited-crypto"], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
if missing_key_check.returncode == 0 or "--key-file" not in missing_key_check.stdout:
    raise SystemExit("native release-check without --key-file should fail closed")

print("production release gates smoke passed")

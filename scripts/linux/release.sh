#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
: "${VENOM_RELEASE_PRIVATE_KEY_FILE:?VENOM_RELEASE_PRIVATE_KEY_FILE must point to the offline Ed25519 private key PEM}"
: "${VENOM_RELEASE_PUBLIC_KEY_FILE:?VENOM_RELEASE_PUBLIC_KEY_FILE must point to the trusted Ed25519 public key PEM}"
[[ -f "$VENOM_RELEASE_PRIVATE_KEY_FILE" ]] || { echo "private key file not found" >&2; exit 2; }
[[ -f "$VENOM_RELEASE_PUBLIC_KEY_FILE" ]] || { echo "public key file not found" >&2; exit 2; }
python3 "$ROOT/tools/final_release_gate.py" --repo-root "$ROOT" --run-smoke-tests
exec python3 "$ROOT/tools/package_release.py" \
  --repo-root "$ROOT" \
  --release-channel stable \
  --sign ed25519 \
  --private-key "$VENOM_RELEASE_PRIVATE_KEY_FILE" \
  --public-key "$VENOM_RELEASE_PUBLIC_KEY_FILE" \
  "$@"

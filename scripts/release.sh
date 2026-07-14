#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-build/release}"
DIST="${DIST:-build/qualified-chess-dist}"
SEED="${SEED:-1350001}"
BROWSER="${BROWSER:-chromium}"
PRIVATE_KEY="${VENOM_RELEASE_PRIVATE_KEY_FILE:-}"
PUBLIC_KEY="${VENOM_RELEASE_PUBLIC_KEY_FILE:-}"
KEY_ID="${VENOM_RELEASE_KEY_ID:-}"
[[ -f "$PRIVATE_KEY" ]] || { echo '[venom] VENOM_RELEASE_PRIVATE_KEY_FILE must point to an Ed25519 private-key PEM.' >&2; exit 2; }
[[ -f "$PUBLIC_KEY" ]] || { echo '[venom] VENOM_RELEASE_PUBLIC_KEY_FILE must point to an Ed25519 public-key PEM.' >&2; exit 2; }
python3 "$ROOT/tools/final_release_gate.py" --repo-root "$ROOT" --run-smoke-tests
if [[ "${SKIP_BUILD:-0}" != "1" ]]; then "$ROOT/scripts/build.sh" Release "$BUILD_DIR"; fi
VENOM="$ROOT/$BUILD_DIR/venom"
python3 "$ROOT/tools/release_qualification.py" --venom "$VENOM" --dist "$DIST" --seed "$SEED" --browser "$BROWSER"
PACKAGE_ARGS=(--repo-root "$ROOT" --build-dir "$ROOT/$BUILD_DIR" --out "$ROOT/build/release-package" --archive zip --release-channel stable --sign ed25519 --private-key "$PRIVATE_KEY" --public-key "$PUBLIC_KEY")
if [[ -n "$KEY_ID" ]]; then PACKAGE_ARGS+=(--key-id "$KEY_ID"); fi
python3 "$ROOT/tools/package_release.py" "${PACKAGE_ARGS[@]}"

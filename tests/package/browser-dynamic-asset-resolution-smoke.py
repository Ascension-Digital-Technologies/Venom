#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import pathlib
import shutil
import subprocess
import sys
import tempfile


def fail(message: str) -> None:
    raise SystemExit(f"browser dynamic asset resolution smoke failed: {message}")


def run(command: list[str], *, cwd: pathlib.Path | None = None) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(command, cwd=cwd, text=True, capture_output=True)
    if result.returncode != 0:
        fail(
            f"command failed ({result.returncode}): {' '.join(command)}\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
        )
    return result


def extract_runtime_functions(runtime_source: str) -> str:
    start_marker = "function normalizeAssetSource(value)"
    end_marker = "\n)JS\";"
    start = runtime_source.find(start_marker)
    end = runtime_source.find(end_marker, start)
    if start < 0 or end < 0:
        fail("unable to locate runtime asset resolver functions")
    return runtime_source[start:end]


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: browser-dynamic-asset-resolution-smoke.py <repo-root> <venom-executable>", file=sys.stderr)
        return 2

    repo = pathlib.Path(sys.argv[1]).resolve()
    venom = pathlib.Path(sys.argv[2]).resolve()
    runtime_cpp = repo / "src/generated/runtime/runtime_js.cpp"
    runtime_template = repo / "src/generated/runtime/javascript/browser_runtime.js"
    asset_runtime_cpp = repo / "src/generated/runtime/browser_asset_runtime_js.cpp"
    wasm_runtime_cpp = repo / "src/generated/runtime/wasm_runtime_js.cpp"
    example = repo / "examples/protected-chess"

    if not venom.is_file():
        fail(f"missing venom executable: {venom}")
    if not runtime_cpp.is_file() or not runtime_template.is_file() or not asset_runtime_cpp.is_file() or not wasm_runtime_cpp.is_file():
        fail("missing runtime generator sources")

    runtime_text = runtime_cpp.read_text(encoding="utf-8")
    runtime_template_text = runtime_template.read_text(encoding="utf-8")
    asset_runtime_text = asset_runtime_cpp.read_text(encoding="utf-8")
    wasm_runtime_text = wasm_runtime_cpp.read_text(encoding="utf-8")
    generator_marker = 'emit_block("__VENOM_BROWSER_ASSET_RUNTIME__", venom::compiler::make_browser_asset_runtime_js());'
    if generator_marker not in runtime_text:
        fail(f"runtime generator is missing marker: {generator_marker}")
    for marker in [
        "installBrowserAssetResolver(route, assetManifest, assetBaseUrl);",
        "setActiveBrowserAssetRoute(runtime.route);",
    ]:
        if marker not in runtime_template_text:
            fail(f"runtime template is missing marker: {marker}")
    install_pos = runtime_template_text.find("installBrowserAssetResolver(route, assetManifest, assetBaseUrl);")
    execute_pos = runtime_template_text.find("const executedScripts = await executeScriptsForRoute", install_pos)
    if install_pos < 0 or execute_pos < 0 or install_pos > execute_pos:
        fail("browser asset resolver is not installed before initial browser scripts execute")
    for marker in [
        "['HTMLImageElement', 'src', resolve]",
        "['HTMLImageElement', 'srcset', resolveSrcsetValue]",
        "['HTMLVideoElement', 'poster', resolve]",
        "Object.defineProperty(globalThis, '__venomResolveAsset'",
    ]:
        if marker not in asset_runtime_text:
            fail(f"asset runtime generator is missing marker: {marker}")
    if "setActiveBrowserAssetRoute(targetRoute);" not in wasm_runtime_text:
        fail("WASM navigation path does not update the active asset route")

    node = shutil.which("node")
    if not node:
        fail("node is required for the resolver behavior test")

    functions = extract_runtime_functions(asset_runtime_text)
    behavior_script = f"""
'use strict';
class Element {{
  constructor() {{ this.attributes = Object.create(null); }}
  setAttribute(name, value) {{ this.attributes[String(name)] = String(value); }}
  getAttribute(name) {{ return this.attributes[String(name)] ?? null; }}
}}
class HTMLImageElement extends Element {{ constructor() {{ super(); this._src = ''; this._srcset = ''; }} }}
Object.defineProperty(HTMLImageElement.prototype, 'src', {{
  configurable: true, enumerable: true,
  get() {{ return this._src; }}, set(value) {{ this._src = String(value); }}
}});
Object.defineProperty(HTMLImageElement.prototype, 'srcset', {{
  configurable: true, enumerable: true,
  get() {{ return this._srcset; }}, set(value) {{ this._srcset = String(value); }}
}});
globalThis.Element = Element;
globalThis.HTMLImageElement = HTMLImageElement;
globalThis.document = {{ baseURI: 'http://127.0.0.1:8080/' }};
{functions}
const manifest = new Map([
  ['img/chesspieces/wikipedia/wP.png', {{ outputName: 'images/img_chesspieces_wikipedia_wP.fc2cccaa5870.png' }}],
  ['img/chesspieces/wikipedia/bK.png', {{ outputName: 'images/img_chesspieces_wikipedia_bK.e33a158ce644.png' }}],
  ['pages/media/card.png', {{ outputName: 'images/pages_media_card.123456789abc.png' }}],
]);
installBrowserAssetResolver({{ sourcePath: 'index.html' }}, manifest, 'http://127.0.0.1:8080/assets/');
const pawn = new HTMLImageElement();
pawn.src = 'img/chesspieces/wikipedia/wP.png';
if (pawn.src !== 'http://127.0.0.1:8080/assets/images/img_chesspieces_wikipedia_wP.fc2cccaa5870.png') throw new Error('image src property was not remapped: ' + pawn.src);
const king = new HTMLImageElement();
king.setAttribute('src', 'img/chesspieces/wikipedia/bK.png?theme=dark#piece');
if (king.getAttribute('src') !== 'http://127.0.0.1:8080/assets/images/img_chesspieces_wikipedia_bK.e33a158ce644.png?theme=dark#piece') throw new Error('setAttribute src was not remapped: ' + king.getAttribute('src'));
const responsive = new HTMLImageElement();
responsive.srcset = 'img/chesspieces/wikipedia/wP.png 1x, img/chesspieces/wikipedia/bK.png 2x';
if (!responsive.srcset.includes('/assets/images/img_chesspieces_wikipedia_wP.fc2cccaa5870.png 1x') || !responsive.srcset.includes('/assets/images/img_chesspieces_wikipedia_bK.e33a158ce644.png 2x')) throw new Error('srcset was not remapped: ' + responsive.srcset);
setActiveBrowserAssetRoute({{ sourcePath: 'pages/game.html' }});
const routed = new HTMLImageElement();
routed.src = 'media/card.png';
if (routed.src !== 'http://127.0.0.1:8080/assets/images/pages_media_card.123456789abc.png') throw new Error('route-relative image was not remapped: ' + routed.src);
const external = new HTMLImageElement();
external.src = 'https://example.com/piece.png';
if (external.src !== 'https://example.com/piece.png') throw new Error('external URL was unexpectedly remapped');
console.log(JSON.stringify({{passed:true, pawn:pawn.src, routed:routed.src}}));
"""
    behavior = run([node, "-e", behavior_script])
    result = json.loads(behavior.stdout.strip())
    if result.get("passed") is not True:
        fail("resolver behavior test did not pass")

    with tempfile.TemporaryDirectory(prefix="venom-dynamic-assets-") as temp:
        dist = pathlib.Path(temp) / "dist"
        run([str(venom), "build", str(example), "--out", str(dist)], cwd=repo)
        image_dir = dist / "assets/images"
        images = sorted(image_dir.glob("*.png")) if image_dir.is_dir() else []
        if len(images) != 12:
            fail(f"expected 12 emitted chess-piece images under assets/images, found {len(images)}")
        if any(path.parent != image_dir for path in images):
            fail("image assets escaped assets/images")
        source_images = sorted((example / "img/chesspieces/wikipedia").glob("*.png"))
        if len(source_images) != 12:
            fail(f"expected 12 source chess-piece images, found {len(source_images)}")
        source_hashes = sorted(hashlib.sha256(path.read_bytes()).hexdigest() for path in source_images)
        output_hashes = sorted(hashlib.sha256(path.read_bytes()).hexdigest() for path in images)
        if source_hashes != output_hashes:
            fail("emitted chess-piece image bytes do not match the source asset set")
        runtime_files = sorted((dist / "assets/runtime").glob("r.*.js"))
        if len(runtime_files) != 1:
            fail(f"expected one runtime JS file, found {len(runtime_files)}")
        # Production runtime identifiers are intentionally hardened. The source-level
        # contract and executable resolver behavior above validate the implementation;
        # strict runtime verification validates the emitted protected distribution.
        run([str(venom), "verify-runtime", str(dist), "--target", "browser", "--require-real-engine"], cwd=repo)

    print("browser dynamic asset resolution smoke: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

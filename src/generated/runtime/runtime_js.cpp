#include "pipeline/js.hpp"

#include <stdexcept>

#include "runtime/runtime_js_template.hpp"

namespace venom::compiler {

std::string make_runtime_js(const SiteGraph& graph, bool protected_release) {
  (void)graph;
  std::string js = runtime_js_template();

  if (protected_release) {
    // Hardened browser releases must never ship the reversible JavaScript section
    // decoder. The WASM runtime owns section authentication, decode, and
    // decompression; absence of that decoder is a fatal boot error.
    const std::string decoder_begin = "function fnvUpdateU64(hash, value) {";
    const std::string decoder_end = "function hexFromBytes(bytes) {";
    const auto begin = js.find(decoder_begin);
    const auto end = js.find(decoder_end);
    if (begin == std::string::npos || end == std::string::npos || end <= begin) {
      throw std::runtime_error("protected runtime JS decoder removal markers are missing");
    }
    js.erase(begin, end - begin);

    const std::string fallback_begin = "function decompressRleV1(input, expectedSize) {";
    const std::string fallback_end = "let venomPackageSectionDecoder = null;";
    const auto compression_begin = js.find(fallback_begin);
    const auto compression_end = js.find(fallback_end);
    if (compression_begin == std::string::npos || compression_end == std::string::npos || compression_end <= compression_begin) {
      throw std::runtime_error("protected runtime JS decompressor removal markers are missing");
    }
    js.erase(compression_begin, compression_end - compression_begin);

    const std::string decode_begin = "function decodeSectionData(storedData, flags, rawSize, name, type, sectionIndex, pkg) {";
    const std::string decode_end = "function isIntegrityAuthSection(section) {";
    const auto section_begin = js.find(decode_begin);
    const auto section_end = js.find(decode_end);
    if (section_begin == std::string::npos || section_end == std::string::npos || section_end <= section_begin) {
      throw std::runtime_error("protected runtime section dispatch replacement markers are missing");
    }
    const std::string wasm_only_dispatch =
        "function decodeSectionData(storedData, flags, rawSize, name, type, sectionIndex, pkg) {\n"
        "  if (!venomPackageSectionDecoder) throw new Error('protected package decoder is unavailable');\n"
        "  return venomPackageSectionDecoder(pkg, sectionIndex, rawSize, name, type);\n"
        "}\n\n";
    js.replace(section_begin, section_end - section_begin, wasm_only_dispatch);
  }

  const auto emit_block = [&](const std::string& marker, const std::string& block) {
    const auto pos = js.find(marker);
    if (pos == std::string::npos) throw std::runtime_error("runtime generation marker missing: " + marker);
    js.replace(pos, marker.size(), block);
  };
  emit_block("__VENOM_BROWSER_ASSET_RUNTIME__", venom::compiler::make_browser_asset_runtime_js());
  emit_block("__VENOM_RUNTIME_ENGINE_FALLBACK_BLOCK__", protected_release
      ? "      throw new Error('host JavaScript fallback is unavailable in protected releases');\n"
      : "      const names = Object.keys(bindings).filter((name) => !sourceDeclaresInjectedBinding(chunk.code, name));\n"
        "      const values = names.map((name) => bindings[name]);\n"
        "      const wrapped = '\"use strict\";\\n' + chunk.code + '\\n//# sourceURL=venom://' + safeSourceUrl(chunk.source);\n"
        "      const fn = new Function(...names, wrapped);\n"
        "      fn(...values);\n"
        "      asyncQueue.settle(request.id, 'fulfilled', { executed: true, context: context.key, fallback: scriptEnginePolicyPlan ? scriptEnginePolicyPlan.fallback : 'host-js-isolated-wrapper' });\n"
        "      recordTurboJsExecution('turbojs.executionRecord', { route: chunk.route, source: chunk.source, order: chunk.order, context: context.key, engine: 'host-js-fallback', fallbackUsed: true, wasmAccepted: false });\n"
        "      recordResultBridge({ route: chunk.route, source: chunk.source, order: chunk.order, context: context.key, ok: true, fallback: true, consoleEvents: consoleEvents.length });\n"
        "      return Object.freeze({ ...chunk, executed: true, engineMode: mode, context: context.key, fallback: scriptEnginePolicyPlan ? scriptEnginePolicyPlan.fallback : 'host-js-isolated-wrapper' });\n");
  emit_block("__VENOM_RUNTIME_LEGACY_FALLBACK_BLOCK__", protected_release
      ? "      throw new Error('legacy host JavaScript fallback is unavailable in protected releases');\n"
      : "      const wrapped = '\"use strict\";\\n' + chunk.code + '\\n//# sourceURL=venom://' + safeSourceUrl(chunk.source);\n"
        "      const fn = new Function('globalThis', 'window', 'document', '__venomRuntime', wrapped);\n"
        "      fn.call(globalThis, globalThis, globalThis.window || globalThis, globalThis.document, bridge);\n"
        "      executed = Object.freeze({ ...chunk, executed: true, fallback: 'legacy-host-js-wrapper' });\n");
  return js;
}



} // namespace venom::compiler

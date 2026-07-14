#include "compiler/services/runtime_modules.hpp"
#include "compiler/pipeline/build.hpp"
#include "compiler/core/version.hpp"

#include "compiler/pipeline/assets.hpp"
#include "compiler/pipeline/capability_analysis.hpp"
#include "compiler/pipeline/css.hpp"
#include "compiler/pipeline/html.hpp"
#include "compiler/pipeline/js.hpp"
#include "compiler/core/profile.hpp"
#include "compiler/core/process.hpp"
#include "compiler/pipeline/security.hpp"
#include "compiler/core/site.hpp"
#include "package/hash.hpp"
#include "package/crypto.hpp"
#include "package/reader.hpp"
#include "package/writer.hpp"
#include "quickjs/abi.hpp"
#include "quickjs/bytecode.hpp"
#include "quickjs/bridge.hpp"
#include "quickjs/engine.hpp"
#include "vm/polymorph.hpp"
#include "vm/encoder.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>

#include "compiler/pipeline/build_support.hpp"

namespace venom::compiler::build_detail {
std::string json_escape(const std::string& value) {
  std::string out;
  for (char c : value) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out += c; break;
    }
  }
  return out;
}

void write_text(const std::filesystem::path& path, const std::string& content) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("failed to write " + path.string());
  }
  out.write(content.data(), static_cast<std::streamsize>(content.size()));
}


std::vector<unsigned char> read_bytes(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to read " + path.string());
  }
  return std::vector<unsigned char>(std::istreambuf_iterator<char>(in), {});
}

std::vector<unsigned char> bytes_from_string(const std::string& value) {
  return {value.begin(), value.end()};
}


std::string metadata_value_from_text(const std::string& text, const std::string& key) {
  const std::string prefix = key + "=";
  std::size_t pos = 0;
  while (pos <= text.size()) {
    const auto end = text.find('\n', pos);
    const auto line = text.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
    if (line.rfind(prefix, 0) == 0) {
      return line.substr(prefix.size());
    }
    if (end == std::string::npos) {
      break;
    }
    pos = end + 1;
  }
  return {};
}

std::string wasm_truth_value(const std::string& key, const std::string& fallback) {
  const auto value = metadata_value_from_text(quickjs_runtime_wasm_provenance_text(), key);
  return value.empty() ? fallback : value;
}

bool wasm_truth_bool(const std::string& key, bool fallback) {
  const auto value = metadata_value_from_text(quickjs_runtime_wasm_provenance_text(), key);
  if (value.empty()) return fallback;
  return value == "true" || value == "yes" || value == "1";
}

void require_real_embedded_quickjs_runtime() {
  const auto provenance = quickjs_runtime_wasm_provenance_text();
  const auto require_value = [&](const std::string& key, const std::string& expected) {
    const auto actual = metadata_value_from_text(provenance, key);
    if (actual != expected) {
      throw std::runtime_error(
          "production browser build refused: embedded QuickJS/WASM provenance " + key +
          "=" + (actual.empty() ? std::string("<missing>") : actual) +
          ", expected " + expected +
          "; run scripts/build-quickjs-wasm to build and embed the verified real runtime");
    }
  };
  require_value("contract_only", "false");
  require_value("scaffold_runtime", "false");
  require_value("real_engine_candidate", "true");
  require_value("full_upstream_quickjs", "true");
  require_value("required_exports_satisfied", "true");
  require_value("fallback_required", "false");
  require_value("source_materialization", "false");
  require_value("native_object_reader", "JS_ReadObject");
  require_value("bytecode_format", "VQJSBC03");
  require_value("required_export_count", "23");
  require_value("release_export_count", "23");
  const auto embedded_exports = metadata_value_from_text(provenance, "exports");
  for (const auto* required_bridge_export : {
           "venom_qjs_bridge_abi",
           "venom_qjs_bridge_input_alloc",
           "venom_qjs_bridge_input_capacity",
           "venom_qjs_bridge_invoke",
           "venom_qjs_bridge_result_ptr",
           "venom_qjs_bridge_result_size",
           "venom_qjs_bridge_release"}) {
    if (embedded_exports.find(required_bridge_export) == std::string::npos) {
      throw std::runtime_error(
          std::string("production browser build refused: embedded QuickJS/WASM provenance is missing protected bridge export ") +
          required_bridge_export + "; run scripts/build-quickjs-wasm to rebuild and embed the current runtime");
    }
  }
  const auto artifact_kind = metadata_value_from_text(provenance, "artifact_kind");
  if (artifact_kind.empty() || artifact_kind == "contract-scaffold") {
    throw std::runtime_error(
        "production browser build refused: embedded QuickJS/WASM is a scaffold; "
        "run scripts/build-quickjs-wasm before building a site");
  }
}


std::uint32_t bridge_protocol_opcode(const std::string& salt, const std::string& label) {
  const auto digest = venom::package::sha256_hex(bytes_from_string(salt + "|bridge-protocol|" + label));
  std::uint32_t value = 0;
  for (std::size_t i = 0; i < 8 && i < digest.size(); ++i) {
    const char c = digest[i];
    const std::uint32_t nibble = (c >= '0' && c <= '9') ? static_cast<std::uint32_t>(c - '0') : static_cast<std::uint32_t>(10 + c - 'a');
    value = (value << 4u) | nibble;
  }
  value |= 0x100u;
  return value;
}
void require_embedded_quickjs_module_bundle_runtime(const JsBridge& bridge) {
  const bool needs_module_loader = !bridge.module_edges.empty();
  if (!needs_module_loader) return;
  const auto provenance = quickjs_runtime_wasm_provenance_text();
  const auto contract = metadata_value_from_text(provenance, "module_bundle_contract");
  const auto dynamic_import = metadata_value_from_text(provenance, "literal_dynamic_import");
  if (contract != "VQJSMB04" || dynamic_import != "true") {
    throw std::runtime_error(
        "protected module graph requires QuickJS/WASM module bundle contract VQJSMB04; "
        "run scripts/build-quickjs-wasm to rebuild and embed the current runtime");
  }
}

std::string named_output(const std::string& stem, const std::string& ext, const std::vector<unsigned char>& bytes, bool hashed) {
  if (!hashed) {
    return stem + ext;
  }
  return stem + "." + short_hash_hex(venom::package::fnv1a64(bytes), 12) + ext;
}

std::string named_output(const std::string& stem, const std::string& ext, const std::string& text, bool hashed) {
  return named_output(stem, ext, bytes_from_string(text), hashed);
}

void replace_all_inplace(std::string& value, const std::string& from, const std::string& to) {
  if (from.empty()) return;
  std::size_t pos = 0;
  while ((pos = value.find(from, pos)) != std::string::npos) {
    value.replace(pos, from.size(), to);
    pos += to.size();
  }
}

std::uint32_t read_wasm_uleb(const std::vector<unsigned char>& bytes, std::size_t& cursor, std::size_t limit) {
  std::uint32_t value = 0;
  unsigned shift = 0;
  while (cursor < limit && shift < 35u) {
    const auto byte = bytes[cursor++];
    value |= static_cast<std::uint32_t>(byte & 0x7fu) << shift;
    if ((byte & 0x80u) == 0u) return value;
    shift += 7u;
  }
  throw std::runtime_error("malformed WASM ULEB128");
}

std::string compact_wasm_export_alias(const std::string& name, std::size_t ordinal, const std::string& salt) {
  std::uint64_t hash = venom::package::fnv1a64(bytes_from_string(name + "|" + salt + "|" + std::to_string(ordinal)));
  static constexpr char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789$_";
  std::string alias(name.size(), '_');
  if (alias.empty()) return alias;
  alias[0] = 'v';
  for (std::size_t i = 1; i < alias.size(); ++i) {
    hash ^= hash >> 12u; hash ^= hash << 25u; hash ^= hash >> 27u;
    alias[i] = alphabet[hash & 63u];
  }
  return alias;
}

std::vector<std::pair<std::string, std::string>> compact_quickjs_wasm_exports(
    std::vector<unsigned char>& bytes, const std::string& salt) {
  std::vector<std::pair<std::string, std::string>> aliases;
  if (bytes.size() < 8u || bytes[0] != 0x00u || bytes[1] != 0x61u || bytes[2] != 0x73u || bytes[3] != 0x6du)
    throw std::runtime_error("invalid QuickJS WASM header");
  std::size_t cursor = 8u;
  while (cursor < bytes.size()) {
    const auto section_id = bytes[cursor++];
    const auto section_size = read_wasm_uleb(bytes, cursor, bytes.size());
    const auto section_end = cursor + section_size;
    if (section_end > bytes.size()) throw std::runtime_error("truncated QuickJS WASM section");
    if (section_id != 7u) { cursor = section_end; continue; }
    const auto count = read_wasm_uleb(bytes, cursor, section_end);
    std::size_t ordinal = 0u;
    for (std::uint32_t i = 0; i < count; ++i) {
      const auto name_size = read_wasm_uleb(bytes, cursor, section_end);
      if (cursor + name_size > section_end) throw std::runtime_error("truncated QuickJS WASM export name");
      const auto name_offset = cursor;
      const std::string name(reinterpret_cast<const char*>(bytes.data() + cursor), name_size);
      cursor += name_size;
      if (cursor >= section_end) throw std::runtime_error("truncated QuickJS WASM export kind");
      ++cursor;
      (void)read_wasm_uleb(bytes, cursor, section_end);
      if (name.rfind("venom_qjs_", 0) == 0) {
        const auto alias = compact_wasm_export_alias(name, ordinal++, salt);
        std::copy(alias.begin(), alias.end(), bytes.begin() + static_cast<std::ptrdiff_t>(name_offset));
        aliases.emplace_back(name, alias);
      }
    }
    return aliases;
  }
  throw std::runtime_error("QuickJS WASM export section missing");
}

void apply_wasm_export_aliases(std::string& js, const std::vector<std::pair<std::string, std::string>>& aliases) {
  for (const auto& [name, alias] : aliases) replace_all_inplace(js, name, alias);
}

void redact_unmapped_quickjs_symbols(std::string& js, const std::string& salt) {
  std::size_t cursor = 0;
  std::size_t ordinal = 0;
  while ((cursor = js.find("venom_qjs_", cursor)) != std::string::npos) {
    std::size_t end = cursor;
    while (end < js.size()) { const unsigned char c = static_cast<unsigned char>(js[end]); if (!(std::isalnum(c) != 0 || js[end] == '_' || js[end] == '$')) break; ++end; }
    const auto name = js.substr(cursor, end - cursor);
    const auto alias = compact_wasm_export_alias(name, ordinal++, salt);
    replace_all_inplace(js, name, alias);
    cursor += alias.size();
  }
}

void redact_quickjs_wasm_abi_table(std::vector<unsigned char>& bytes, const std::string& salt) {
  static const std::string marker = "VENOM_QJS_RUNTIME_ABI_V12\n";
  const auto it = std::search(bytes.begin(), bytes.end(), marker.begin(), marker.end());
  if (it == bytes.end()) return;
  auto cursor = static_cast<std::size_t>(std::distance(bytes.begin(), it));
  std::uint64_t state = venom::package::fnv1a64(bytes_from_string("abi-table|" + salt));
  static constexpr char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  while (cursor < bytes.size() && bytes[cursor] != 0u) {
    if (bytes[cursor] != '\n' && bytes[cursor] != '\t' && bytes[cursor] >= 0x20u && bytes[cursor] <= 0x7eu) {
      state ^= state >> 12u; state ^= state << 25u; state ^= state >> 27u;
      bytes[cursor] = static_cast<unsigned char>(alphabet[state % 62u]);
    }
    ++cursor;
  }
}

bool js_identifier_char(char ch) {
  const auto c = static_cast<unsigned char>(ch);
  return std::isalnum(c) != 0 || ch == '_' || ch == '$';
}

bool js_regex_prefix(char ch) {
  switch (ch) {
    case 0: case '(': case '[': case '{': case ':': case ';': case ',':
    case '=': case '!': case '?': case '&': case '|': case '+': case '-':
    case '*': case '%': case '^': case '~': case '<': case '>':
      return true;
    default:
      return false;
  }
}

std::string minify_release_js(const std::string& input) {
  enum class State { Normal, Single, Double, Regex, RegexClass, LineComment, BlockComment };
  State state = State::Normal;
  std::string out;
  out.reserve(input.size());
  bool escaped = false;
  bool pending_space = false;
  char previous_significant = 0;

  auto emit_pending_space = [&](char next) {
    if (!pending_space) return;
    const char prev = out.empty() ? 0 : out.back();
    if ((js_identifier_char(prev) && js_identifier_char(next)) ||
        (prev == '+' && next == '+') || (prev == '-' && next == '-') ||
        (prev == '/' && next == '/') || (prev == '/' && next == '*')) {
      out.push_back(' ');
    }
    pending_space = false;
  };

  for (std::size_t i = 0; i < input.size(); ++i) {
    const char ch = input[i];
    const char next = i + 1 < input.size() ? input[i + 1] : 0;
    if (state == State::LineComment) {
      if (ch == '\n' || ch == '\r') { state = State::Normal; pending_space = true; }
      continue;
    }
    if (state == State::BlockComment) {
      if (ch == '*' && next == '/') { state = State::Normal; ++i; pending_space = true; }
      continue;
    }
    if (state == State::Single || state == State::Double) {
      out.push_back(ch);
      if (escaped) { escaped = false; continue; }
      if (ch == '\\') { escaped = true; continue; }
      if ((state == State::Single && ch == '\'') || (state == State::Double && ch == '"')) {
        state = State::Normal;
        previous_significant = ch;
      }
      continue;
    }
    if (state == State::Regex || state == State::RegexClass) {
      out.push_back(ch);
      if (escaped) { escaped = false; continue; }
      if (ch == '\\') { escaped = true; continue; }
      if (state == State::Regex && ch == '[') { state = State::RegexClass; continue; }
      if (state == State::RegexClass && ch == ']') { state = State::Regex; continue; }
      if (state == State::Regex && ch == '/') {
        while (i + 1 < input.size() && std::isalpha(static_cast<unsigned char>(input[i + 1])) != 0) out.push_back(input[++i]);
        state = State::Normal;
        previous_significant = '/';
      }
      continue;
    }

    if (std::isspace(static_cast<unsigned char>(ch)) != 0) { pending_space = true; continue; }
    if (ch == '/' && next == '/') { state = State::LineComment; ++i; continue; }
    if (ch == '/' && next == '*') { state = State::BlockComment; ++i; continue; }
    emit_pending_space(ch);
    if (ch == '\'') { state = State::Single; out.push_back(ch); escaped = false; continue; }
    if (ch == '"') { state = State::Double; out.push_back(ch); escaped = false; continue; }
    if (ch == '/' && js_regex_prefix(previous_significant)) { state = State::Regex; out.push_back(ch); escaped = false; continue; }
    out.push_back(ch);
    previous_significant = ch;
  }
  return out;
}

std::string harden_release_js_asset(std::string js) {
  replace_all_inplace(js, "sourceURL=venom://", "sourceURL=v://");
  replace_all_inplace(js, "sourceURL=venom-qjs-module://", "sourceURL=vq://");
  replace_all_inplace(js, "host-js-fallback", "hjsf-denied");
  replace_all_inplace(js, "host-js-isolated-wrapper", "hjs-wrapper-denied");
  replace_all_inplace(js, "legacy-host-js-wrapper", "legacy-hjs-denied");
  // Collapse high-signal diagnostic terminology in protected assets. The same
  // deterministic mapping is applied to runtime and engine modules.
  const std::vector<std::pair<std::string, std::string>> opaque_names = {
      {"executionJournal", "xj"}, {"execution_journal", "xj"},
      {"snapshotRecord", "xr"}, {"snapshot_record", "xr"},
      {"capabilityLedger", "cl"}, {"capability_ledger", "cl"},
      {"determinismLedger", "dl"}, {"determinism_ledger", "dl"},
      {"replayValidation", "rv"}, {"releaseAcceptance", "ra"},
      {"policySealAudit", "psa"}, {"runtimeDenylist", "rd"},
      {"fallback_required", "fr"},
      {"executionCommit", "ec"}, {"checkpointPolicy", "cp"},
      {"resolverAudit", "za"}, {"hostIoDenyTrace", "hdt"},
  };
  for (const auto& [from, to] : opaque_names) replace_all_inplace(js, from, to);
  return minify_release_js(js);
}


std::filesystem::path find_js_hardener_script() {
  auto cursor = std::filesystem::current_path();
  for (int depth = 0; depth < 8; ++depth) {
    const auto candidate = cursor / "tools" / "js-hardener" / "harden.mjs";
    if (std::filesystem::exists(candidate)) return candidate;
    if (!cursor.has_parent_path() || cursor.parent_path() == cursor) break;
    cursor = cursor.parent_path();
  }
  return {};
}

std::string ast_harden_release_js(const std::string& kind, const std::string& js) {
  const auto script = find_js_hardener_script();
  if (script.empty()) {
    throw std::runtime_error(
        "protected release JS hardener is unavailable; run scripts/setup-js-hardener and build from the repository root");
  }
  const auto package_dir = script.parent_path();
  if (!std::filesystem::exists(package_dir / "node_modules" / "terser") ||
      !std::filesystem::exists(package_dir / "node_modules" / "javascript-obfuscator")) {
    throw std::runtime_error(
        "protected release JS hardener dependencies are missing; run scripts/setup-js-hardener");
  }

  const auto nonce = short_hash_hex(
      venom::package::fnv1a64(bytes_from_string(kind + "|" + js)), 16);
  const auto temp_dir = std::filesystem::temp_directory_path() / ("venom-js-hardener-" + nonce);
  const auto input_path = temp_dir / "input.js";
  const auto output_path = temp_dir / "output.js";
  std::filesystem::remove_all(temp_dir);
  std::filesystem::create_directories(temp_dir);
  write_text(input_path, js);
  const auto seed = static_cast<std::uint32_t>(
      venom::package::fnv1a64(bytes_from_string("obfuscate|" + kind + "|" + nonce)) & 0xffffffffu);
  const int status = run_process("node", {
      script.string(), input_path.string(), output_path.string(), kind, std::to_string(seed)});
  if (status != 0 || !std::filesystem::exists(output_path)) {
    std::filesystem::remove_all(temp_dir);
    throw std::runtime_error(
        "protected release JS hardener failed for " + kind + " (exit " + std::to_string(status) + ")");
  }
  const auto output_bytes = read_bytes(output_path);
  std::filesystem::remove_all(temp_dir);
  if (output_bytes.empty()) {
    throw std::runtime_error("protected release JS hardener produced empty output for " + kind);
  }
  return std::string(output_bytes.begin(), output_bytes.end());
}

bool redact_release_metadata(const Profile& profile) {
  return profile.kind == ProfileKind::Prod || profile.strip_debug_metadata;
}

std::string protected_route_label(const Profile& profile, const JsChunk& chunk) {
  if (!redact_release_metadata(profile)) return chunk.route;
  return "r_" + short_hash_hex(
      venom::package::fnv1a64(bytes_from_string("route|" + chunk.route)), 16);
}

std::string protected_source_label(const Profile& profile, const JsChunk& chunk) {
  if (!redact_release_metadata(profile)) return chunk.source;
  return "s_" + short_hash_hex(
      venom::package::fnv1a64(bytes_from_string(
          "source|" + chunk.route + "|" + chunk.source + "|" + std::to_string(chunk.order))),
      16);
}

std::size_t protected_source_size(const Profile& profile, const JsChunk& chunk) {
  return redact_release_metadata(profile) ? 0u : chunk.code.size();
}

std::uint64_t protected_source_hash(const Profile& profile, const JsChunk& chunk) {
  if (redact_release_metadata(profile)) return 0u;
  return venom::package::fnv1a64(bytes_from_string(chunk.code));
}

void validate_protected_js_asset(const std::string& name, const std::string& js) {
  static const std::vector<std::string> forbidden = {
      "new Function", "eval(", "host-js-fallback", "host-js-isolated-wrapper",
      "legacy-host-js-wrapper", "__venomDisabledSourceEval", "__venomDisabledEval"};
  for (const auto& token : forbidden) {
    if (js.find(token) != std::string::npos) {
      throw std::runtime_error("protected JS asset '" + name + "' contains forbidden execution token: " + token);
    }
  }
}

bool is_code_or_markup(const SiteFile& file) {
  return file.extension == ".html" || file.extension == ".htm" ||
         file.extension == ".css" || file.extension == ".js" || file.extension == ".mjs";
}

bool is_html_route_file(const SiteFile& file) {
  return file.extension == ".html" || file.extension == ".htm";
}

std::string route_shell_asset_prefix(const std::filesystem::path& relative_html_path) {
  const auto parent = relative_html_path.parent_path();
  if (parent.empty() || parent == ".") {
    return "assets/";
  }
  std::string prefix;
  for (const auto& part : parent) {
    if (part.empty() || part == ".") {
      continue;
    }
    prefix += "../";
  }
  return prefix + "assets/";
}

void emit_html_route_shells(const SiteGraph& graph,
                            const std::filesystem::path& output_dir,
                            const std::string& loader_name,
                            const std::string& style_name) {
  for (const auto& file : graph.files) {
    if (!is_html_route_file(file)) {
      continue;
    }
    const std::filesystem::path relative(file.relative);
    const auto prefix = route_shell_asset_prefix(relative);
    write_text(output_dir / relative, make_bootstrap_html(graph, prefix + loader_name, prefix + style_name));
  }
}

std::string make_service_worker_js() {
  return "// Venom preview service worker stub.\n"
         "// The runtime is package-driven; this file exists so preview servers do not 404 stale /sw.js checks.\n"
         "self.addEventListener('install', event => { self.skipWaiting(); });\n"
         "self.addEventListener('activate', event => { event.waitUntil(self.clients.claim()); });\n";
}


} // namespace venom::compiler::build_detail

#include "compiler/runtime_modules.hpp"
#include "compiler/build.hpp"
#include "compiler/version.hpp"

#include "compiler/assets.hpp"
#include "compiler/capability_analysis.hpp"
#include "compiler/css.hpp"
#include "compiler/html.hpp"
#include "compiler/js.hpp"
#include "compiler/profile.hpp"
#include "compiler/process.hpp"
#include "compiler/security.hpp"
#include "compiler/site.hpp"
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

namespace venom::compiler {

namespace {
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
  return profile.kind == ProfileKind::Release || profile.strip_debug_metadata;
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


struct PendingPackageSection {
  venom::package::SectionType type = venom::package::SectionType::Manifest;
  std::uint32_t flags = venom::package::SectionFlagNone;
  std::string name;
  std::vector<unsigned char> data;
};

struct ReleaseBuildPolicy {
  bool release_like = false;
  bool has_scripts = false;
  bool backend_is_scaffold = true;
  bool backend_is_real_browser = false;
  bool fallback_allowed = false;
  bool fallback_denied = false;
  bool unsafe_release_fallback = false;
  std::string backend;
  std::string decision;
  std::string reason;
};

ReleaseBuildPolicy resolve_release_build_policy(const Profile& profile,
                                                const BuildOptions& options,
                                                std::size_t script_count) {
  ReleaseBuildPolicy policy;
  policy.release_like = profile.kind == ProfileKind::Release || profile.fail_closed || options.strict_release;
  policy.has_scripts = script_count != 0;
  policy.backend = options.quickjs_backend.empty() ? "scaffold" : options.quickjs_backend;
  policy.backend_is_scaffold = policy.backend == "scaffold";
  policy.backend_is_real_browser = policy.backend == "wasm-real";
  policy.fallback_allowed = options.allow_host_fallback && !policy.release_like && !options.deny_host_fallback && !options.strict_release;
  policy.fallback_denied = options.deny_host_fallback || options.strict_release || profile.fail_closed || (profile.kind == ProfileKind::Release && !policy.fallback_allowed);
  policy.unsafe_release_fallback = policy.release_like && policy.fallback_allowed && policy.has_scripts && policy.backend_is_scaffold;

  if (policy.release_like && policy.has_scripts && policy.backend_is_scaffold && !policy.fallback_allowed) {
    policy.decision = "deny-build";
    policy.reason = "script-bearing release package requires host-JS fallback because QuickJS backend is scaffold";
  } else if (policy.release_like && policy.has_scripts && policy.backend == "native") {
    policy.decision = "deny-build";
    policy.reason = "native QuickJS is compile-time only and cannot execute browser scripts in release output";
  } else if (policy.release_like && policy.has_scripts && policy.backend == "wasm-real") {
    policy.decision = "allow-build";
    policy.reason = "script-bearing release package uses fail-closed QuickJS/WASM backend contract";
  } else if (policy.unsafe_release_fallback) {
    policy.decision = "allow-unsafe-fallback";
    policy.reason = "explicit host fallback override accepted for compatibility package";
  } else {
    policy.decision = "allow-build";
    policy.reason = policy.has_scripts ? "fallback policy is compatible with non-release profile" : "site has no packaged script chunks";
  }
  return policy;
}

void enforce_release_build_policy(const ReleaseBuildPolicy& policy, std::size_t script_count) {
  if (policy.decision == "deny-build") {
    std::ostringstream err;
    err << "release build denied: site contains " << script_count
        << " script chunk" << (script_count == 1 ? "" : "s")
        << ", quickjs backend '" << policy.backend << "' cannot satisfy strict release execution ("
        << policy.reason << "). Protected and release profiles cannot enable host fallback; use --quickjs-backend wasm-real for the protected QuickJS/WASM execution backend.";
    throw std::runtime_error(err.str());
  }
}

std::string make_release_build_policy_metadata(const Profile& profile,
                                               const BuildOptions& options,
                                               const ReleaseBuildPolicy& policy,
                                               std::size_t script_count) {
  std::ostringstream out;
  out << "VENOM_RELEASE_BUILD_POLICY_V1\n"
      << "version=1\n"
      << "package_version=" << venom::package::kVersion << "\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << options.runtime << "\n"
      << "quickjs_backend=" << policy.backend << "\n"
      << "script_count=" << script_count << "\n"
      << "release_like=" << (policy.release_like ? "true" : "false") << "\n"
      << "fallback_allowed=" << (policy.fallback_allowed ? "true" : "false") << "\n"
      << "fallback_denied=" << (policy.fallback_denied ? "true" : "false") << "\n"
      << "unsafe_release_fallback=" << (policy.unsafe_release_fallback ? "true" : "false") << "\n"
      << "decision=" << policy.decision << "\n"
      << "reason=" << policy.reason << "\n"
      << "strict_release_option=" << (options.strict_release ? "true" : "false") << "\n"
      << "allow_host_fallback_option=" << (options.allow_host_fallback ? "true" : "false") << "\n"
      << "deny_host_fallback_option=" << (options.deny_host_fallback ? "true" : "false") << "\n";
  return out.str();
}




std::string selected_section_sealer_name(const Profile& profile, const std::string& crypto_provider) {
  if (!profile.crypto_provider_ready || profile.kind == ProfileKind::Debug) {
    return "none";
  }
  if (crypto_provider == "libsodium") {
    return venom::package::libsodium_aead_provider_name();
  }
  return "venom-aead-section-v1";
}

venom::package::SectionCryptoProvider selected_writer_crypto_provider(const std::string& crypto_provider) {
  if (crypto_provider == "libsodium") {
    return venom::package::SectionCryptoProvider::LibsodiumXChaCha20Poly1305;
  }
  return venom::package::SectionCryptoProvider::RuntimeSoftware;
}

std::string stored_section_name_for_metadata(const Profile& profile, venom::package::SectionType type, const std::string& name) {
  if (profile.strip_debug_metadata && profile.kind != ProfileKind::Debug && venom::package::should_redact_section_name(type)) {
    return venom::package::protected_section_alias(type, name);
  }
  return name;
}


std::string make_package_binding_token(bool deterministic, const std::string& seed) {
  if (deterministic) {
    return venom::package::sha256_hex(bytes_from_string("venom-binding-token-v1|" + seed));
  }
  std::vector<unsigned char> bytes(32u);
  std::random_device rd;
  for (auto& byte : bytes) {
    byte = static_cast<unsigned char>(rd() & 0xffu);
  }
  return venom::package::sha256_hex(bytes);
}


std::string package_section_order_digest(const Profile& profile, const std::vector<PendingPackageSection>& sections) {
  std::ostringstream canonical;
  for (const auto& section : sections) {
    canonical << static_cast<std::uint32_t>(section.type) << '\t'
              << stored_section_name_for_metadata(profile, section.type, section.name) << '\t'
              << section.data.size() << '\t'
              << venom::package::sha256_hex(section.data) << '\n';
  }
  const auto text = canonical.str();
  return venom::package::sha256_hex(bytes_from_string(text));
}

std::string make_package_layout_metadata(const Profile& profile,
                                         const BuildOptions& options,
                                         const venom::vm::PolymorphicPlan& poly,
                                         const std::vector<PendingPackageSection>& sections,
                                         std::uint32_t max_padding) {
  std::ostringstream out;
  out << "VENOM_PACKAGE_LAYOUT_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << options.runtime << "\n"
      << "security_target=" << options.security_target << "\n"
      << "enabled=" << (profile.polymorphic ? "true" : "false") << "\n"
      << "section_shuffle=" << (profile.shuffle_sections ? "true" : "false") << "\n"
      << "payload_jitter=true\n"
      << "payload_jitter_max=" << max_padding << "\n"
      << "layout_seed_commitment=" << poly.seed_commitment << "\n"
      << "section_name_redaction=" << ((profile.strip_debug_metadata && profile.kind != ProfileKind::Debug) ? "true" : "false") << "\n"
      << "section_count_before_layout=" << sections.size() << "\n"
      << "section_order_digest=" << package_section_order_digest(profile, sections) << "\n";
  return out.str();
}


void append_u32_local(std::vector<unsigned char>& out, std::uint32_t value) {
  out.push_back(static_cast<unsigned char>(value & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 8u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 16u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 24u) & 0xffu));
}

std::vector<unsigned char> make_quickjs_abi_fingerprint() {
  // VQAF0001 + version + FNV-1a fingerprint of the one supported release ABI.
  std::vector<unsigned char> out;
  out.insert(out.end(), {'V','Q','A','F','0','0','0','1'});
  append_u32_local(out, 1u);
  append_u32_local(out, 0x3b9ace76u);
  append_u32_local(out, 12u);
  append_u32_local(out, 16u);
  return out;
}

std::vector<unsigned char> make_release_diversification_table(const venom::vm::PolymorphicPlan& poly) {
  // Binary-only release contract. Logical host-call and field identifiers are
  // mapped to per-build wire identifiers, preventing stable cross-build ABI
  // signatures while retaining deterministic builds when requested.
  constexpr std::uint32_t kHostCallCount = 89u;
  constexpr std::uint32_t kResultFieldCount = 8u;
  std::vector<std::uint32_t> host_ids(kHostCallCount);
  std::vector<std::uint32_t> field_ids(kResultFieldCount);
  for (std::uint32_t i = 0; i < kHostCallCount; ++i) host_ids[i] = i + 1u;
  for (std::uint32_t i = 0; i < kResultFieldCount; ++i) field_ids[i] = i;
  venom::vm::DiversificationRng host_rng(poly, "quickjs-host-wire-map");
  venom::vm::DiversificationRng field_rng(poly, "quickjs-result-field-map");
  std::shuffle(host_ids.begin(), host_ids.end(), host_rng);
  std::shuffle(field_ids.begin(), field_ids.end(), field_rng);

  std::vector<unsigned char> out;
  out.insert(out.end(), {'V','R','D','I','V','0','0','3'});
  append_u32_local(out, 3u);
  append_u32_local(out, poly.seed ^ 0xD1A5EED3u);
  append_u32_local(out, kHostCallCount);
  append_u32_local(out, kResultFieldCount);
  for (std::uint32_t logical = 0; logical < kHostCallCount; ++logical) {
    append_u32_local(out, logical + 1u);
    append_u32_local(out, host_ids[logical]);
  }
  for (std::uint32_t logical = 0; logical < kResultFieldCount; ++logical) {
    append_u32_local(out, logical);
    append_u32_local(out, field_ids[logical]);
  }
  return out;
}

std::vector<unsigned char> make_single_route_bytecode_section(const venom::vm::Program& program,
                                                              const venom::vm::PolymorphicPlan& poly) {
  const auto encoded = venom::vm::encode_program(program, poly);
  std::vector<unsigned char> out;
  out.insert(out.end(), {'V', 'B', 'C', 'O', 'D', 'E', '0', '3'});
  append_u32_local(out, 1);
  append_u32_local(out, 16);
  append_u32_local(out, static_cast<std::uint32_t>(program.size()));
  append_u32_local(out, 0);
  out.insert(out.end(), encoded.begin(), encoded.end());
  return out;
}

std::vector<unsigned char> encode_route_script_bundle(const std::vector<JsChunk>& chunks, bool opaque_metadata) {
  struct Entry {
    std::uint32_t route_offset = 0;
    std::uint32_t route_size = 0;
    std::uint32_t source_offset = 0;
    std::uint32_t source_size = 0;
    std::uint32_t code_offset = 0;
    std::uint32_t code_size = 0;
    std::uint32_t order = 0;
    std::uint32_t flags = 0;
    std::uint32_t kind = 0;
    std::uint32_t reserved = 0;
  };
  std::vector<Entry> entries;
  std::vector<unsigned char> text_blob;
  std::vector<unsigned char> code_blob;
  std::vector<venom::quickjs::ModuleSourceRecord> module_sources;
  entries.reserve(chunks.size());
  module_sources.reserve(chunks.size());
  for (const auto& chunk : chunks) {
    if ((chunk.flags & JsChunkModule) == 0u) continue;
    const std::string compile_name = opaque_metadata
        ? ("s_" + short_hash_hex(venom::package::fnv1a64(bytes_from_string("source|" + chunk.route + "|" + chunk.source + "|" + std::to_string(chunk.order))), 16))
        : chunk.source;
    module_sources.push_back({chunk.source, compile_name, chunk.code});
  }
  for (const auto& chunk : chunks) {
    Entry entry;
    // Route labels remain executable routing keys. Hiding them without a separate
    // authenticated route-id map prevents the browser runtime from selecting the
    // route's script bundle. Source labels are hardened independently below.
    const std::string route_label = chunk.route;
    const std::string source_label = opaque_metadata
        ? ("s_" + short_hash_hex(venom::package::fnv1a64(bytes_from_string("source|" + chunk.route + "|" + chunk.source + "|" + std::to_string(chunk.order))), 16))
        : chunk.source;
    entry.route_offset = static_cast<std::uint32_t>(text_blob.size());
    entry.route_size = static_cast<std::uint32_t>(route_label.size());
    text_blob.insert(text_blob.end(), route_label.begin(), route_label.end());
    entry.source_offset = static_cast<std::uint32_t>(text_blob.size());
    entry.source_size = static_cast<std::uint32_t>(source_label.size());
    text_blob.insert(text_blob.end(), source_label.begin(), source_label.end());
    const bool browser_side = (chunk.flags & JsChunkBrowser) != 0u;
    const bool is_module = (chunk.flags & JsChunkModule) != 0u;
    std::vector<unsigned char> payload;
    if (browser_side) {
      // Browser-realm chunks must remain browser-executable source. Encoding
      // them as QuickJS bytecode leaves chunk.code empty at runtime and causes
      // the browser executor to silently skip the script.
      payload.assign(chunk.code.begin(), chunk.code.end());
    } else {
      payload = venom::quickjs::compile_native_quickjs_bytecode(
          chunk.code,
          source_label,
          is_module,
          opaque_metadata,
          is_module ? &module_sources : nullptr);
    }
    entry.code_offset = static_cast<std::uint32_t>(code_blob.size());
    entry.code_size = static_cast<std::uint32_t>(payload.size());
    code_blob.insert(code_blob.end(), payload.begin(), payload.end());
    entry.order = chunk.order;
    entry.flags = browser_side ? (chunk.flags & ~JsChunkBytecodeEncoded)
                               : (chunk.flags | JsChunkBytecodeEncoded);
    entry.kind = chunk.kind;
    entries.push_back(entry);
  }
  std::vector<unsigned char> out;
  out.insert(out.end(), {'V', 'J', 'S', 'B', '0', '0', '0', '6'});
  append_u32_local(out, 1);
  append_u32_local(out, static_cast<std::uint32_t>(entries.size()));
  append_u32_local(out, static_cast<std::uint32_t>(text_blob.size()));
  append_u32_local(out, 0);
  for (const auto& entry : entries) {
    append_u32_local(out, entry.route_offset);
    append_u32_local(out, entry.route_size);
    append_u32_local(out, entry.source_offset);
    append_u32_local(out, entry.source_size);
    append_u32_local(out, entry.code_offset);
    append_u32_local(out, entry.code_size);
    append_u32_local(out, entry.order);
    append_u32_local(out, entry.flags);
    append_u32_local(out, entry.kind);
    append_u32_local(out, entry.reserved);
  }
  out.insert(out.end(), text_blob.begin(), text_blob.end());
  out.insert(out.end(), code_blob.begin(), code_blob.end());
  return out;
}

std::string lazy_route_section_name(const std::string& route) {
  return "route-chunk." + short_hash_hex(venom::package::fnv1a64(bytes_from_string(route)), 16) + ".vmbc";
}

std::string lazy_script_section_name(const std::string& route) {
  return "script-chunk." + short_hash_hex(venom::package::fnv1a64(bytes_from_string("script|" + route)), 16) + ".vjsb";
}

struct LazySectionPlanRow {
  std::string route;
  std::string route_section;
  std::uint32_t instruction_count = 0;
  std::uint32_t route_bytecode_size = 0;
  std::string script_section;
  std::uint32_t script_count = 0;
};

std::string make_lazy_sections_metadata(const Profile& profile,
                                        const BuildOptions& options,
                                        const std::vector<LazySectionPlanRow>& rows) {
  std::uint32_t route_count = 0;
  std::uint32_t script_route_count = 0;
  for (const auto& row : rows) {
    if (!row.route_section.empty()) ++route_count;
    if (!row.script_section.empty()) ++script_route_count;
  }
  std::ostringstream out;
  out << "VENOM_LAZY_SECTIONS_V1\n"
      << "version=1\n"
      << "enabled=true\n"
      << "mode=route-scoped-decode-on-first-use\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << options.runtime << "\n"
      << "security_target=" << options.security_target << "\n"
      << "verify_on_decode=true\n"
      << "route_count=" << route_count << "\n"
      << "script_route_count=" << script_route_count << "\n";
  for (const auto& row : rows) {
    if (!row.route_section.empty()) {
      out << "route\t" << row.route << "\t" << row.route_section << "\t"
          << row.instruction_count << "\t" << row.route_bytecode_size << "\n";
    }
    if (!row.script_section.empty()) {
      out << "script\t" << row.route << "\t" << row.script_section << "\t" << row.script_count << "\n";
    }
  }
  return out.str();
}


std::string release_threat_model_id(const BuildOptions& options) {
  if (options.security_target == "native" || options.crypto_provider == "libsodium" || options.require_audited_crypto) {
    return "native-private-aead-v1";
  }
  return "browser-client-protection-v1";
}

std::string release_secret_model(const BuildOptions& options) {
  if (options.security_target == "native" || options.crypto_provider == "libsodium" || options.require_audited_crypto) {
    return "external-32-byte-package-key-required";
  }
  return "browser-runtime-no-hidden-secret";
}

std::string make_release_profile_metadata(const Profile& profile,
                                          const BuildOptions& options,
                                          const ReleaseBuildPolicy& release_policy,
                                          std::size_t script_count) {
  const bool native_target = options.security_target == "native" || options.crypto_provider == "libsodium" || options.require_audited_crypto;
  const bool browser_target = !native_target;
  const bool audited_crypto = options.crypto_provider == "libsodium";
  std::ostringstream out;
  out << "VENOM_RELEASE_PROFILE_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "security_target=" << (native_target ? "native" : (options.security_target == "browser" ? "browser" : options.security_target)) << "\n"
      << "runtime_mode=" << options.runtime << "\n"
      << "quickjs_backend=" << release_policy.backend << "\n"
      << "quickjs_execution_backend=" << (release_policy.backend == "wasm-real" ? "quickjs-wasm-real" : release_policy.backend) << "\n"
      << "wasm_backend_required=" << ((release_policy.backend == "wasm-real" && script_count != 0u) ? "true" : "false") << "\n"
      << "host_js_fallback_allowed=" << ((release_policy.fallback_allowed && release_policy.backend != "wasm-real") ? "true" : "false") << "\n"
      << "script_count=" << script_count << "\n"
      << "threat_model=" << release_threat_model_id(options) << "\n"
      << "secret_material_model=" << release_secret_model(options) << "\n"
      << "crypto_provider=" << selected_section_sealer_name(profile, options.crypto_provider) << "\n"
      << "audited_crypto=" << (audited_crypto ? "true" : "false") << "\n"
      << "browser_executable=" << (browser_target ? "true" : "false") << "\n"
      << "native_private_crypto=" << (audited_crypto ? "true" : "false") << "\n"
      << "runtime_decrypts_package=" << (browser_target ? "true" : "false") << "\n"
      << "external_key_required=" << (native_target ? "true" : "false") << "\n"
      << "release_check_required=true\n"
      << "debug_metadata_allowed=false\n"
      << "external_manifest_allowed=false\n"
      << "source_preserving_js_allowed=false\n"
      << "remote_script_policy=build-time-vendor\n"
      << "unvendored_remote_scripts_allowed=false\n"
      << "runtime_remote_script_network_required=false\n"
      << "package_binding_required=true\n"
      << "layout_polymorphism_required=true\n"
      << "lazy_sections_required=true\n"
      << "readable_internal_section_names_allowed=false\n"
      << "legacy_sealer_allowed=false\n"
      << "server_secret_claim=false\n"
      << "tamper_response=fail-closed\n"
      << "native_key_handling=" << (native_target ? "key-file-or-env-only" : "not-applicable") << "\n"
      << "browser_secret_warning=" << (browser_target ? "browser-protect cannot hide secrets from the browser runtime" : "not-browser-runnable") << "\n";
  return out.str();
}

std::string make_package_binding_metadata(const Profile& profile,
                                          const BuildOptions& options,
                                          const std::string& binding_token,
                                          const std::string& runtime_name,
                                          const std::string& runtime,
                                          const std::string& runtime_wasm_name,
                                          const std::vector<unsigned char>& runtime_wasm_bytes,
                                          const std::string& style_name,
                                          const std::string& css,
                                          const std::string& quickjs_engine_name,
                                          const std::string& quickjs_engine_module,
                                          const std::string& quickjs_wasm_name,
                                          const std::vector<unsigned char>& quickjs_wasm_bytes,
                                          const std::string& worker_name,
                                          const std::string& worker_runtime) {
  struct BoundAsset {
    std::string role;
    std::string name;
    std::string digest;
  };
  std::vector<BoundAsset> assets;
  assets.push_back({"runtime", runtime_name, venom::package::sha256_hex(bytes_from_string(runtime))});
  if (!runtime_wasm_name.empty()) assets.push_back({"runtime_wasm", runtime_wasm_name, venom::package::sha256_hex(runtime_wasm_bytes)});
  assets.push_back({"style", style_name, venom::package::sha256_hex(bytes_from_string(css))});
  assets.push_back({"quickjs_engine", quickjs_engine_name, venom::package::sha256_hex(bytes_from_string(quickjs_engine_module))});
  assets.push_back({"quickjs_wasm", quickjs_wasm_name, venom::package::sha256_hex(quickjs_wasm_bytes)});
  if (!worker_name.empty()) assets.push_back({"worker", worker_name, venom::package::sha256_hex(bytes_from_string(worker_runtime))});

  std::ostringstream out;
  out << "VENOM_PACKAGE_BINDING_V1\n"
      << "version=1\n"
      << "enabled=true\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << options.runtime << "\n"
      << "security_target=" << options.security_target << "\n"
      << "crypto_provider=" << selected_section_sealer_name(profile, options.crypto_provider) << "\n"
      << "hardened=" << (profile.runtime_hardened || profile.fail_closed ? "true" : "false") << "\n"
      << "binding_token_sha256=" << venom::package::sha256_hex(bytes_from_string(binding_token)) << "\n"
      << "asset_count=" << assets.size() << "\n";
  for (const auto& asset : assets) {
    out << "asset\t" << asset.role << "\t" << asset.name << "\t" << asset.digest << "\n";
  }
  return out.str();
}

std::string make_integrity_metadata(const Profile& profile, const std::string& runtime_mode, const std::vector<PendingPackageSection>& sections, const std::string& crypto_provider) {
  std::ostringstream out;
  out << "VENOM_INTEGRITY_V1\n"
      << "provider=sha256-software-v1\n"
      << "scope=decoded-sections\n"
      << "aead_provider=" << selected_section_sealer_name(profile, crypto_provider) << "\n"
      << "section_sealer=" << selected_section_sealer_name(profile, crypto_provider) << "\n"
      << "section_name_redaction=" << ((profile.strip_debug_metadata && profile.kind != ProfileKind::Debug) ? "true" : "false") << "\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "wasm_runtime=" << (runtime_mode == "wasm" ? "true" : "false") << "\n"
      << "section_count=" << sections.size() << "\n";
  for (const auto& section : sections) {
    out << "section\t"
        << static_cast<std::uint32_t>(section.type) << "\t"
        << stored_section_name_for_metadata(profile, section.type, section.name) << "\t"
        << section.data.size() << "\t"
        << venom::package::sha256_hex(section.data) << "\n";
  }
  return out.str();
}




bool package_feature_required_in_release(const PendingPackageSection& section) {
  if (section.type == venom::package::SectionType::Asset ||
      section.type == venom::package::SectionType::Css ||
      section.type == venom::package::SectionType::DomTemplates) {
    return false;
  }
  if (section.name == "profile-diagnostics.txt" || section.name == "script-diagnostics.txt" ||
      section.name == "bundle-preview.js" || section.name == "quickjs-probe.txt" ||
      section.name == "quickjs-bridge-plan.txt") {
    return false;
  }
  return true;
}

std::string make_package_features_metadata(const Profile& profile,
                                           const std::string& runtime_mode,
                                           const std::vector<PendingPackageSection>& sections,
                                           const std::string& crypto_provider) {
  std::ostringstream out;
  std::uint32_t feature_count = 0;
  std::uint32_t release_required_count = 0;
  for (const auto& section : sections) {
    if (section.type == venom::package::SectionType::PackageFeatures ||
        (section.type == venom::package::SectionType::Integrity && section.name == "integrity-auth.vsig")) {
      continue;
    }
    ++feature_count;
    if (package_feature_required_in_release(section)) {
      ++release_required_count;
    }
  }

  out << "VENOM_PACKAGE_FEATURES_V2\n"
      << "version=2\n"
      << "package_version=" << venom::package::kVersion << "\n"
      << "runtime_abi=" << venom::package::kRuntimeAbi << "\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "flag_model=profile-flags-plus-feature-table\n"
      << "legacy_flags_scope=coarse-profile-and-runtime-only\n"
      << "integrity_provider_ready=" << (profile.integrity_metadata ? "true" : "false") << "\n"
      << "aead_provider_ready=" << (profile.aead_provider_ready ? "true" : "false") << "\n"
      << "section_sealer=" << selected_section_sealer_name(profile, crypto_provider) << "\n"
      << "sealed_sections=" << ((profile.crypto_provider_ready && profile.kind != ProfileKind::Debug) ? "true" : "false") << "\n"
      << "section_name_redaction=" << ((profile.strip_debug_metadata && profile.kind != ProfileKind::Debug) ? "true" : "false") << "\n"
      << "signature_provider_ready=false\n"
      << "production_metadata_profile=" << ((profile.strip_debug_metadata && profile.kind != ProfileKind::Debug) ? "minimal-v1" : "diagnostic-v1") << "\n"
      << "feature_count=" << feature_count << "\n"
      << "required_in_release_count=" << release_required_count << "\n";

  std::uint32_t id = 1;
  for (const auto& section : sections) {
    if (section.type == venom::package::SectionType::PackageFeatures ||
        (section.type == venom::package::SectionType::Integrity && section.name == "integrity-auth.vsig")) {
      continue;
    }
    out << "feature\t"
        << id++ << "\t"
        << venom::package::section_type_name(section.type) << "\t"
        << "1\t"
        << (package_feature_required_in_release(section) ? "true" : "false") << "\t"
        << static_cast<std::uint32_t>(section.type) << "\t"
        << stored_section_name_for_metadata(profile, section.type, section.name) << "\t"
        << venom::package::sha256_hex(section.data) << "\n";
  }
  return out.str();
}

std::string make_host_bridge_metadata(const Profile& profile,
                                      const std::string& runtime_mode,
                                      const JsBridge& js_bridge) {
  auto contains_any = [](const std::string& text, std::initializer_list<const char*> needles) {
    for (const char* needle : needles) {
      if (text.find(needle) != std::string::npos) return true;
    }
    return false;
  };

  bool has_scripts = !js_bridge.chunks.empty();
  bool needs_fetch = false;
  bool needs_timers = false;
  bool needs_events = false;
  for (const auto& chunk : js_bridge.chunks) {
    const std::string& code = chunk.code;
    needs_fetch = needs_fetch || contains_any(code, {"fetch(", "XMLHttpRequest", "WebSocket("});
    needs_timers = needs_timers || contains_any(code, {"setTimeout(", "clearTimeout(", "setInterval(", "clearInterval("});
    needs_events = needs_events || contains_any(code, {"addEventListener(", "removeEventListener(", ".onclick", ".onsubmit", ".onchange", ".oninput"});
  }
  // Script execution itself requires the event queue only when an event surface is present.
  // Static/script-free sites therefore receive the smallest bridge contract.
  const std::uint32_t capability_count = 6u + (needs_fetch ? 3u : 0u) + (needs_timers ? 2u : 0u) + (needs_events ? 2u : 0u);

  std::ostringstream out;
  out << "VENOM_HOST_BRIDGE_V14\n"
      << "version=14\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "host_call_count=" << capability_count << "\n"
      << "capability_count=" << capability_count << "\n"
      << "capability_model=application-specialized-schema-v1\n"
      << "unknown_host_call=deny\n"
      << "generic_global_lookup=false\n"
      << "generic_property_traversal=false\n"
      << "generic_method_invoke=false\n"
      << "source_strings_cross_boundary=false\n"
      << "function_objects_cross_boundary=false\n"
      << "application_specialized=true\n"
      << "scripts_present=" << (has_scripts ? "true" : "false") << "\n"
      << "fetch_enabled=" << (needs_fetch ? "true" : "false") << "\n"
      << "timers_enabled=" << (needs_timers ? "true" : "false") << "\n"
      << "events_enabled=" << (needs_events ? "true" : "false") << "\n"
      << "max_request_bytes=65536\n"
      << "max_response_bytes=1048576\n"
      << "max_string_bytes=32768\n"
      << "max_array_items=4096\n"
      << "max_object_depth=16\n"
      << "max_outstanding_async=256\n";

  auto capability = [&](std::uint32_t id, const char* name, const char* request, const char* response,
                        std::uint32_t max_request, std::uint32_t max_response, const char* surface) {
    out << "capability\t" << id << "\t" << name << "\t" << request << "\t" << response
        << "\t" << max_request << "\t" << max_response << "\n";
    out << "host_call\t" << id << "\t" << name << "\t" << surface << "\t" << runtime_mode << "\n";
  };

  capability(1u, "dom.createElement", "string:tag", "object:handle", 1024u, 64u, "dom");
  capability(2u, "dom.setAttribute", "handle,string:name,string:value", "void", 33792u, 0u, "dom");
  capability(3u, "dom.appendChild", "handle:parent,handle:child", "void", 16u, 0u, "dom");
  capability(4u, "event.bindInline", "handle,string:event,u32:binding", "void", 1040u, 0u, "events");
  capability(5u, "asset.resolve", "string:path", "string:url", 32768u, 32768u, "assets");
  capability(6u, "route.navigate", "string:path,u32:mode", "void", 32772u, 0u, "routes");
  if (needs_fetch) {
    capability(7u, "fetch.request", "string:url,string:method,bytes:body", "u32:request_id", 65536u, 4u, "network");
    capability(8u, "fetch.response", "u32:request_id,u32:status,bytes:body", "void", 1048576u, 0u, "network");
    capability(9u, "fetch.error", "u32:request_id,string:error", "void", 32772u, 0u, "network");
  }
  if (needs_timers) {
    capability(10u, "timer.schedule", "u32:delay,u32:token", "void", 8u, 0u, "timers");
    capability(11u, "timer.cancel", "u32:token", "void", 4u, 0u, "timers");
  }
  if (needs_events) {
    capability(12u, "event.queue", "u32:binding,bytes:event", "void", 65536u, 0u, "events");
    capability(13u, "event.dispatch", "u32:binding,bytes:event", "void", 65536u, 0u, "events");
  }
  return out.str();
}

std::string make_fetch_bridge_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_FETCH_BRIDGE_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "request_host_call=fetch.request\n"
      << "response_host_call=fetch.response\n"
      << "error_host_call=fetch.error\n"
      << "queue=async-host-queue.vahq\n"
      << "cache_policy=browser-default\n"
      << "credentials_policy=same-origin\n"
      << "body_bridge=uint8array\n"
      << "text_response=true\n"
      << "json_response=true\n";
  return out.str();
}

std::string make_timer_bridge_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_TIMER_BRIDGE_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "schedule_host_call=timer.schedule\n"
      << "cancel_host_call=timer.cancel\n"
      << "queue=async-host-queue.vahq\n"
      << "clock=browser-event-loop\n"
      << "timer_id_bits=32\n"
      << "max_active=128\n"
      << "max_scheduled_per_route=512\n"
      << "minimum_delay_ms=0\n"
      << "wasm_response_boundary=planned\n"
      << "quickjs_timer_boundary=planned\n";
  return out.str();
}

std::string make_event_queue_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_EVENT_QUEUE_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "queue_host_call=event.queue\n"
      << "dispatch_host_call=event.dispatch\n"
      << "inline_event_bindings=true\n"
      << "max_records=256\n"
      << "max_dispatches_per_route=1024\n"
      << "record_payload=eventName|attrName|route|targetTag\n"
      << "wasm_event_boundary=dom-op-bindEvent\n"
      << "quickjs_event_boundary=planned\n";
  return out.str();
}

std::string make_quickjs_bridge_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_BRIDGE_V9\n"
      << "version=10\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "mode=engine-backend-select\n"
      << "call_host_call=quickjs.call\n"
      << "result_host_call=quickjs.result\n"
      << "queue=async-host-queue.vahq\n"
      << "script_isolation=route-scoped\n"
      << "bytecode_input=chunk-records\n"
      << "chunk_metadata=quickjs-chunks.vqbc\n"
      << "capability_policy=script-policy.vscp\n"
      << "request_record=quickjs.call\n"
      << "result_record=quickjs.result\n"
      << "wasm_engine=module-loader\n"
      << "native_engine=compile-time-probe\n"
      << "engine_metadata=quickjs-engine.vqse\n"
      << "script_engine_policy=script-engine-policy.vsep\n"
      << "legacy_fallback_policy=module-fallback-policy\n"
      << "context_lifecycle=quickjs-context-lifecycle.vqcl\n"
      << "host_capabilities=host-capabilities.vhcap\n"
      << "adapter_diagnostics=quickjs-adapter-diagnostics.vqad\n"
      << "wasm_runtime=quickjs-wasm-runtime.vqwr\n"
      << "source_transfer=quickjs-source-transfer.vqst\n"
      << "console_bridge=quickjs-console-bridge.vqcb\n"
      << "execution_records=quickjs-execution-records.vqer\n"
      << "result_bridge=quickjs-result-bridge.vqrb\n"
      << "fallback_policy=quickjs-fallback-policy.vqfp\n"
      << "runtime_abi=quickjs-runtime-abi.vqra\n"
      << "host_imports=quickjs-host-imports.vqhi\n"
      << "heap_limits=quickjs-heap-limits.vqhl\n"
      << "script_buffer=quickjs-script-buffer.vqsb\n"
      << "console_callback_abi=quickjs-console-abi.vqca\n"
      << "parity_probe=quickjs-parity-probe.vqpp\n"
      << "execution_lifecycle=quickjs-execution-lifecycle.vqel\n"
      << "script_buffer_policy=quickjs-script-buffer-policy.vqsp\n"
      << "context_limit_policy=quickjs-context-limit-policy.vqlp\n"
      << "host_call_dispatch=quickjs-host-call-dispatch.vqhd\n"
      << "parity_contract=quickjs-parity-contract.vqpc\n"
      << "release_failclosed=quickjs-release-failclosed.vqfc\n"
      << "module_graph=quickjs-module-graph.vqmg\n"
      << "module_execution=quickjs-module-execution.vqmx\n"
      << "module_cache=quickjs-module-cache.vqmc\n"
      << "resolver_audit=quickjs-resolver-audit.vqra\n"
      << "interop_fallback=quickjs-interop-fallback.vqif\n"
      << "execution_pipeline=quickjs-execution-pipeline.vqxp\n"
      << "script_records=quickjs-script-records.vqsr\n"
      << "eval_results=quickjs-eval-results.vqev\n"
      << "console_capture=quickjs-console-capture.vqcc\n"
      << "failure_reports=quickjs-failure-reports.vqfr\n";
  return out.str();
}

std::string make_async_host_queue_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_ASYNC_HOST_QUEUE_V10\n"
      << "version=10\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "request_id_bits=32\n"
      << "state=pending|fulfilled|rejected\n"
      << "max_pending=128\n"
      << "max_completed=128\n"
      << "max_host_calls_per_route=8192\n"
      << "max_concurrent_fetches=16\n"
      << "max_dom_handles=4096\n"
      << "max_fetch_response_bytes=1048576\n"
      << "max_fetch_request_bytes=65536\n"
      << "max_timer_delay_ms=86400000\n"
      << "max_route_lifetime_ms=86400000\n"
      << "teardown=cancel-timers|abort-fetches|reject-pending|destroy-contexts\n"
      << "host_call=fetch.request\n"
      << "host_call=fetch.response\n"
      << "host_call=fetch.error\n"
      << "host_call=timer.schedule\n"
      << "host_call=timer.cancel\n"
      << "host_call=event.queue\n"
      << "host_call=event.dispatch\n"
      << "host_call=quickjs.call\n"
      << "host_call=quickjs.result\n"
      << "host_call=quickjs.chunk\n"
      << "host_call=quickjs.bytecodeBoundary\n"
      << "host_call=script.scopeCreate\n"
      << "host_call=script.chunkStart\n"
      << "host_call=script.chunkFinish\n"
      << "host_call=script.policyCheck\n"
      << "host_call=quickjs.engineBoot\n"
      << "host_call=quickjs.contextCreate\n"
      << "host_call=quickjs.executeChunk\n"
      << "host_call=quickjs.executionResult\n"
      << "host_call=quickjs.contextReuse\n"
      << "host_call=quickjs.contextDestroy\n"
      << "host_call=quickjs.hostCapabilityInject\n"
      << "host_call=quickjs.adapterStatus\n"
      << "host_call=quickjs.moduleContextCreate\n"
      << "host_call=quickjs.moduleContextDestroy\n"
      << "host_call=quickjs.executionRecord\n"
      << "host_call=quickjs.resultBridge\n"
      << "host_call=quickjs.fallbackPolicyCheck\n"
      << "host_call=quickjs.consoleEvent\n"
      << "host_call=quickjs.consoleFlush\n"
      << "host_call=quickjs.wasmResultDecode\n"
      << "host_call=quickjs.executionSnapshot\n"
      << "host_call=script.fallbackDecision\n"
      << "host_call=script.engineFallback\n"
      << "host_call=script.capabilityBind\n"
      << "host_call=quickjs.abiTable\n"
      << "host_call=quickjs.contextSetLimits\n"
      << "host_call=quickjs.contextHeapAccounting\n"
      << "host_call=quickjs.scriptBufferAlloc\n"
      << "host_call=quickjs.scriptBufferFree\n"
      << "host_call=quickjs.consoleCallbackAbi\n"
      << "host_call=quickjs.consoleEventDrain\n"
      << "host_call=quickjs.hostImportTable\n"
      << "host_call=quickjs.wasmParityProbe\n"
      << "host_call=quickjs.releaseFallbackDeny\n"
      << "timer_bridge=enabled\n"
      << "event_queue=enabled\n"
      << "quickjs_bridge=engine-module\n"
      << "script_isolation=route-scoped\n"
      << "wasm_request_boundary=planned\n"
      << "quickjs_request_boundary=engine-records\n";
  return out.str();
}


std::string make_script_isolation_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  out << "VENOM_SCRIPT_ISOLATION_V4\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "mode=route-scoped\n"
      << "scope_key=route|source|order\n"
      << "global_policy=shared-browser-global-with-route-wrapper\n"
      << "document_access=bridge-parameter\n"
      << "window_access=bridge-parameter\n"
      << "quickjs_boundary=engine-bootstrap\n"
      << "engine_context=route-scoped\n"
      << "context_lifecycle=tracked-reuse-destroy\n"
      << "engine_fallback=deny-host-js-source-eval\n"
      << "adapter_context_lifecycle=quickjs-context-lifecycle.vqcl\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\t" << protected_source_size(profile, chunk) << "\t" << js_flags_summary(chunk.flags) << "\n";
  }
  return out.str();
}

std::string make_script_policy_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  out << "VENOM_SCRIPT_POLICY_V4\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "default_capabilities=console,document,window,fetch,timers,events,quickjs-engine-bootstrap,host-capability-injection\n"
      << "remote_scripts=vendored-package-bytecode\n"
      << "module_scripts=engine-fallback-wrapper\n"
      << "inline_scripts=route-wrapper\n"
      << "policy_check_host_call=script.policyCheck\n"
      << "chunk_start_host_call=script.chunkStart\n"
      << "chunk_finish_host_call=script.chunkFinish\n"
      << "engine_fallback=deny-host-js-source-eval\n"
      << "adapter_context_lifecycle=quickjs-context-lifecycle.vqcl\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "policy\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tcapabilities=console,document,window,fetch,timers,events\n";
  }
  return out.str();
}

std::string make_quickjs_chunk_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const auto protected_count = static_cast<std::size_t>(std::count_if(
      bridge.chunks.begin(), bridge.chunks.end(),
      [](const JsChunk& chunk) { return (chunk.flags & JsChunkBrowser) == 0u; }));
  std::ostringstream out;
  out << "VENOM_QUICKJS_CHUNKS_V7\n"
      << "version=7\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "mode=engine-input\n"
      << "bytecode_provider=source-chunks-v1\n"
      << "bytecode_chunk_section=route-scoped-script-chunk.*.vjsb\n"
      << "request_host_call=quickjs.chunk\n"
      << "bytecode_boundary_host_call=quickjs.bytecodeBoundary\n"
      << "engine_execute_host_call=quickjs.executeChunk\n"
      << "engine_fallback=deny-host-js-source-eval\n"
      << "adapter_context_lifecycle=quickjs-context-lifecycle.vqcl\n"
      << "chunk_count=" << protected_count << "\n";
  for (const auto& chunk : bridge.chunks) {
    if ((chunk.flags & JsChunkBrowser) != 0u) continue;
    out << "qjs_chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk) << "\tflags=" << js_flags_summary(chunk.flags) << "\n";
  }
  return out.str();
}


std::string make_quickjs_engine_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_ENGINE_V7\n"
      << "version=7\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "mode=engine-backend-replacement-path\n"
      << "engine=quickjs-backend-selector\n"
      << "browser_engine=quickjs-runtime-wasm-replacement-candidate\n"
      << "native_engine=quickjs-compile-time-probe\n"
      << "context_model=route-scoped-reusable\n"
      << "context_lifecycle=quickjs-context-lifecycle.vqcl\n"
      << "host_capability_bindings=host-capabilities.vhcap\n"
      << "bytecode_chunks=route-scoped-script-chunk.*.vjsb\n"
      << "capability_policy=script-engine-policy.vsep\n"
      << "context_create_host_call=quickjs.contextCreate\n"
      << "execute_host_call=quickjs.executeChunk\n"
      << "result_host_call=quickjs.executionResult\n"
      << "fallback_host_call=quickjs.moduleFallback\n"
      << "wasm_runtime=quickjs-runtime.wasm\n"
      << "wasm_runtime_asset=quickjs-runtime.wasm\n"
      << "source_transfer=quickjs-source-transfer.vqst\n"
      << "console_bridge=quickjs-console-bridge.vqcb\n"
      << "module_graph=quickjs-module-graph.vqmg\n"
      << "module_execution=quickjs-module-execution.vqmx\n"
      << "module_cache=quickjs-module-cache.vqmc\n"
      << "resolver_audit=quickjs-resolver-audit.vqra\n"
      << "interop_fallback=quickjs-interop-fallback.vqif\n"
      << "execution_pipeline=quickjs-execution-pipeline.vqxp\n"
      << "script_records=quickjs-script-records.vqsr\n"
      << "eval_results=quickjs-eval-results.vqev\n"
      << "console_capture=quickjs-console-capture.vqcc\n"
      << "failure_reports=quickjs-failure-reports.vqfr\n"
      << "snapshot_policy=quickjs-snapshot-policy.vqsk\n"
      << "snapshot_records=quickjs-snapshot-records.vqsn\n"
      << "replay_validation=quickjs-replay-validation.vqrv\n"
      << "determinism_ledger=quickjs-determinism-ledger.vqdl\n"
      << "audit_seal=quickjs-audit-seal.vqas\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "engine_chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk) << "\tflags=" << js_flags_summary(chunk.flags) << "\n";
  }
  return out.str();
}

std::string make_quickjs_engine_module_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge, const std::string& engine_asset_name, const std::string& wasm_asset_name) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_ENGINE_MODULE_V7\n"
      << "version=10\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "asset_name=" << engine_asset_name << "\n"
      << "wasm_asset_name=" << wasm_asset_name << "\n"
      << "module_format=esm\n"
      << "loader=dynamic-import\n"
      << "export=createVenomQuickJsEngineModule\n"
      << "execution_mode=quickjs-wasm-abi12-upstream-global-host-api-shims\n"
      << "context_model=route-scoped-reusable\n"
      << "context_lifecycle=quickjs-context-lifecycle.vqcl\n"
      << "host_capability_bindings=host-capabilities.vhcap\n"
      << "fallback=fail-closed-no-host-js\n"
      << "bytecode_chunks=route-scoped-script-chunk.*.vjsb\n"
      << "wasm_loader=fetch-instantiate\n"
      << "interpreter_dispatch=enabled\n"
      << "source_decode_fallback=denied-in-protect\n"
      << "bytecode_transfer_api=venom_qjs_script_buffer_alloc|venom_qjs_script_buffer_ptr|venom_qjs_bytecode_validate|venom_qjs_execute_bytecode|venom_qjs_interpreter_dispatch_count|venom_qjs_result_ptr\n"
      << "console_bridge=quickjs-console-bridge.vqcb\n"
      << "module_graph=quickjs-module-graph.vqmg\n"
      << "module_execution=quickjs-module-execution.vqmx\n"
      << "module_cache=quickjs-module-cache.vqmc\n"
      << "resolver_audit=quickjs-resolver-audit.vqra\n"
      << "interop_fallback=quickjs-interop-fallback.vqif\n"
      << "execution_pipeline=quickjs-execution-pipeline.vqxp\n"
      << "script_records=quickjs-script-records.vqsr\n"
      << "eval_results=quickjs-eval-results.vqev\n"
      << "console_capture=quickjs-console-capture.vqcc\n"
      << "failure_reports=quickjs-failure-reports.vqfr\n"
      << "snapshot_policy=quickjs-snapshot-policy.vqsk\n"
      << "snapshot_records=quickjs-snapshot-records.vqsn\n"
      << "replay_validation=quickjs-replay-validation.vqrv\n"
      << "determinism_ledger=quickjs-determinism-ledger.vqdl\n"
      << "audit_seal=quickjs-audit-seal.vqas\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "module_chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk) << "\tflags=" << js_flags_summary(chunk.flags) << "\n";
  }
  return out.str();
}

std::string make_quickjs_wasm_runtime_metadata(const Profile& profile, const std::string& runtime_mode, const std::string& wasm_asset_name) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_WASM_RUNTIME_V10\n"
      << "version=10\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "asset_name=" << wasm_asset_name << "\n"
      << "abi=" << venom::quickjs::kRuntimeAbiVersion << "\n"
      << "package_version=" << venom::quickjs::kRuntimePackageVersion << "\n"
      << "execution_mode=quickjs-wasm-abi12-upstream-global-host-api-shims\n"
      << "runtime_implementation=" << wasm_truth_value("runtime_implementation", "quickjs-wasm-contract-scaffold") << "\n"
      << "runtime_claim=" << wasm_truth_value("runtime_claim", "contract-boundary") << "\n"
      << "contract_only=" << (wasm_truth_bool("contract_only", true) ? "true" : "false") << "\n"
      << "scaffold_runtime=" << (wasm_truth_bool("scaffold_runtime", true) ? "true" : "false") << "\n"
      << "real_engine_candidate=" << (wasm_truth_bool("real_engine_candidate", false) ? "true" : "false") << "\n"
      << "full_upstream_quickjs=" << (wasm_truth_bool("full_upstream_quickjs", false) ? "true" : "false") << "\n"
      << "fallback_required=" << (wasm_truth_bool("fallback_required", false) ? "true" : "false") << "\n"
      << "finish_blocker=" << wasm_truth_value("finish_blocker", "replace_quickjs_runtime_scaffold_with_upstream_quickjs_wasm") << "\n"
      << "artifact_kind=" << wasm_truth_value("artifact_kind", "contract-scaffold") << "\n"
      << "wasm_sha256=" << quickjs_runtime_wasm_sha256() << "\n"
      << "required_exports_satisfied=" << (wasm_truth_bool("required_exports_satisfied", false) ? "true" : "false") << "\n"
      << "missing_export_count=" << wasm_truth_value("missing_export_count", "0") << "\n"
      << "exports=venom_qjs_backend_kind,venom_qjs_backend_ready,venom_qjs_bytecode_expected_source_hash32,venom_qjs_bytecode_payload_size,venom_qjs_bytecode_record_executed,venom_qjs_bytecode_record_hash32,venom_qjs_bytecode_record_size,venom_qjs_bytecode_record_source_hash32,venom_qjs_bytecode_validate,venom_qjs_console_clear,venom_qjs_console_event_count,venom_qjs_console_event_ptr,venom_qjs_console_event_size,venom_qjs_context_alloc,venom_qjs_context_free,venom_qjs_context_heap_limit,venom_qjs_context_heap_used,venom_qjs_context_set_limits,venom_qjs_context_stack_limit,venom_qjs_engine_abi,venom_qjs_engine_state,venom_qjs_engine_trap_code,venom_qjs_engine_version,venom_qjs_exception_clear,venom_qjs_exception_code,venom_qjs_exception_message_ptr,venom_qjs_exception_message_size,venom_qjs_exception_ptr,venom_qjs_exception_size,venom_qjs_execute_bytecode,venom_qjs_execute_source,venom_qjs_execution_record_ptr,venom_qjs_execution_record_size,venom_qjs_fallback_required,venom_qjs_host_call_count,venom_qjs_host_call_dispatch,venom_qjs_host_call_known,venom_qjs_interop_fallback_required,venom_qjs_interpreter_ready,venom_qjs_module_execute,venom_qjs_module_execution_count,venom_qjs_module_record_ptr,venom_qjs_module_record_size,venom_qjs_parity_probe,venom_qjs_real_engine_candidate,venom_qjs_release_status,venom_qjs_result_ptr,venom_qjs_result_size,venom_qjs_script_buffer_alloc,venom_qjs_script_buffer_alloc_count,venom_qjs_script_buffer_capacity,venom_qjs_script_buffer_expected_hash,venom_qjs_script_buffer_free,venom_qjs_script_buffer_free_count,venom_qjs_script_buffer_ptr,venom_qjs_script_buffer_set_expected_hash,venom_qjs_script_buffer_size,venom_qjs_status_code,venom_qjs_status_record_ptr,venom_qjs_status_record_size,venom_qjs_upstream_quickjs_ready,venom_qjs_wasm_native_stack_capacity\n"
      << "fallback=deny-host-js-source-eval\n"
      << "abi_contract=quickjs-wasm-abi12-runtime\n"
      << "status_exports=venom_qjs_status_code,venom_qjs_status_record_ptr,venom_qjs_status_record_size,venom_qjs_release_status\n"
      << "limit_exports=venom_qjs_context_heap_limit,venom_qjs_context_heap_used,venom_qjs_context_stack_limit\n"
      << "bytecode_validation_exports=venom_qjs_bytecode_validate,venom_qjs_bytecode_record_hash32,venom_qjs_bytecode_payload_size,venom_qjs_bytecode_expected_source_hash32\n"
      << "interpreter_exports=venom_qjs_interpreter_ready,venom_qjs_interpreter_contract_ptr,venom_qjs_interpreter_dispatch_count,venom_qjs_interpreter_opcode_count,venom_qjs_global_slot_count,venom_qjs_last_global_write_hash,venom_qjs_interpreter_semantic_pass_count,venom_qjs_interpreter_property_write_count,venom_qjs_interpreter_builtin_probe_count,venom_qjs_interpreter_console_call_count,venom_qjs_interpreter_semantic_record_ptr,venom_qjs_interpreter_semantic_record_size,venom_qjs_interpreter_semantic_record_hash,venom_qjs_global_slot_record_ptr,venom_qjs_global_slot_record_size,venom_qjs_global_slot_record_hash,venom_qjs_upstream_quickjs_ready,venom_qjs_upstream_parity_record_ptr,venom_qjs_upstream_parity_record_size,venom_qjs_upstream_parity_record_hash,venom_qjs_upstream_feature_count,venom_qjs_upstream_builtin_count,venom_qjs_upstream_object_model_score,venom_qjs_upstream_exception_model_score,venom_qjs_upstream_module_model_score,venom_qjs_upstream_object_record_ptr,venom_qjs_upstream_object_record_size,venom_qjs_upstream_object_record_hash,venom_qjs_upstream_exception_record_ptr,venom_qjs_upstream_exception_record_size,venom_qjs_upstream_exception_record_hash,venom_qjs_upstream_module_record_ptr,venom_qjs_upstream_module_record_size,venom_qjs_upstream_module_record_hash,venom_qjs_upstream_runtime_bridge_score,venom_qjs_upstream_bytecode_semantics_record_ptr,venom_qjs_upstream_bytecode_semantics_record_size,venom_qjs_upstream_bytecode_semantics_record_hash,venom_qjs_upstream_bytecode_semantics_score,venom_qjs_upstream_lexical_scope_record_ptr,venom_qjs_upstream_lexical_scope_record_size,venom_qjs_upstream_lexical_scope_record_hash,venom_qjs_upstream_closure_record_ptr,venom_qjs_upstream_closure_record_size,venom_qjs_upstream_closure_record_hash,venom_qjs_upstream_iterator_record_ptr,venom_qjs_upstream_iterator_record_size,venom_qjs_upstream_iterator_record_hash,venom_qjs_upstream_intrinsic_record_ptr,venom_qjs_upstream_intrinsic_record_size,venom_qjs_upstream_intrinsic_record_hash,venom_qjs_upstream_intrinsic_semantics_score,venom_qjs_upstream_property_descriptor_record_ptr,venom_qjs_upstream_property_descriptor_record_size,venom_qjs_upstream_property_descriptor_record_hash,venom_qjs_upstream_prototype_chain_record_ptr,venom_qjs_upstream_prototype_chain_record_size,venom_qjs_upstream_prototype_chain_record_hash,venom_qjs_upstream_call_frame_record_ptr,venom_qjs_upstream_call_frame_record_size,venom_qjs_upstream_call_frame_record_hash\n"
      << "engine_backend=quickjs-engine-backend.vqeb\n";
  return out.str();
}

std::string make_quickjs_wasm_execution_metadata(const Profile& profile,
                                                const BuildOptions& options,
                                                const ReleaseBuildPolicy& policy,
                                                const JsBridge& bridge,
                                                const std::string& wasm_asset_name) {
  const bool wasm_real = policy.backend == "wasm-real";
  const bool host_fallback_allowed = policy.fallback_allowed && !wasm_real;
  const auto protected_count = static_cast<std::size_t>(std::count_if(
      bridge.chunks.begin(), bridge.chunks.end(),
      [](const JsChunk& chunk) { return (chunk.flags & JsChunkBrowser) == 0u; }));
  std::ostringstream out;
  out << "VENOM_QJS_WASM_EXECUTION_V2\n"
      << "version=2\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << options.runtime << "\n"
      << "security_target=" << options.security_target << "\n"
      << "enabled=" << (wasm_real ? "true" : "false") << "\n"
      << "selected_backend=" << (wasm_real ? "quickjs-wasm-real" : policy.backend) << "\n"
      << "backend_claim=" << (wasm_real ? wasm_truth_value("runtime_claim", "quickjs-wasm-contract") : policy.backend) << "\n"
      << "runtime_implementation=" << wasm_truth_value("runtime_implementation", "quickjs-wasm-contract-scaffold") << "\n"
      << "contract_only=" << (wasm_truth_bool("contract_only", true) ? "true" : "false") << "\n"
      << "scaffold_runtime=" << (wasm_truth_bool("scaffold_runtime", true) ? "true" : "false") << "\n"
      << "real_engine_candidate=" << (wasm_truth_bool("real_engine_candidate", false) ? "true" : "false") << "\n"
      << "full_upstream_quickjs=" << (wasm_truth_bool("full_upstream_quickjs", false) ? "true" : "false") << "\n"
      << "artifact_kind=" << wasm_truth_value("artifact_kind", "contract-scaffold") << "\n"
      << "required_exports_satisfied=" << (wasm_truth_bool("required_exports_satisfied", false) ? "true" : "false") << "\n"
      << "missing_export_count=" << wasm_truth_value("missing_export_count", "0") << "\n"
      << "backend_contract=quickjs-wasm-abi12-upstream-global-host-api-shims-v7\n"
      << "wasm_runtime_asset=" << wasm_asset_name << "\n"
      << "bytecode_format=VQJSBC03\n"
      << "bytecode_source_preserving=false\n"
      << "decode_boundary=wasm-owned-bytecode-abi12-upstream-semantics\n"
      << "source_eval_fallback=false\n"
      << "interpreter_dispatch=enabled\n"
      << "source_eval_in_runtime=false\n"
      << "host_js_fallback_allowed=" << (host_fallback_allowed ? "true" : "false") << "\n"
      << "silent_fallback_allowed=false\n"
      << "require_engine_module=true\n"
      << "require_wasm_asset_bound=true\n"
      << "require_result_commit=true\n"
      << "release_fail_closed=true\n"
      << "runtime_abi=" << venom::quickjs::kRuntimeAbiVersion << "\n"
      << "status_code_export=venom_qjs_status_code\n"
      << "status_record_export=venom_qjs_status_record_ptr|venom_qjs_status_record_size\n"
      << "bytecode_validate_export=venom_qjs_bytecode_validate\n"
      << "bytecode_hash_export=venom_qjs_bytecode_record_hash32\n"
      << "bytecode_payload_export=venom_qjs_bytecode_payload_size\n"
      << "backend_contract_export=venom_qjs_backend_contract_ptr|venom_qjs_backend_contract_size\n"
      << "runtime_limits_export=venom_qjs_runtime_limits_ptr|venom_qjs_runtime_limits_size\n"
      << "context_report_export=venom_qjs_context_report_ptr|venom_qjs_context_report_size\n"
      << "interpreter_ready_export=venom_qjs_interpreter_ready\n"
      << "interpreter_contract_export=venom_qjs_interpreter_contract_ptr|venom_qjs_interpreter_contract_size\n"
      << "interpreter_dispatch_export=venom_qjs_interpreter_dispatch_count\n"
      << "semantic_runtime=enabled\n"
      << "upstream_quickjs_bridge=enabled\n"
      << "upstream_interpreter_core=quickjs-upstream-global-host-api-shims-v7\n"
      << "upstream_parity_record_export=venom_qjs_upstream_parity_record_ptr|venom_qjs_upstream_parity_record_size\n"
      << "upstream_runtime_record_exports=venom_qjs_upstream_object_record_ptr|venom_qjs_upstream_exception_record_ptr|venom_qjs_upstream_module_record_ptr|venom_qjs_upstream_runtime_bridge_score|venom_qjs_upstream_bytecode_semantics_record_ptr|venom_qjs_upstream_lexical_scope_record_ptr|venom_qjs_upstream_closure_record_ptr|venom_qjs_upstream_iterator_record_ptr|venom_qjs_upstream_intrinsic_record_ptr|venom_qjs_upstream_property_descriptor_record_ptr|venom_qjs_upstream_prototype_chain_record_ptr|venom_qjs_upstream_call_frame_record_ptr\n"
      << "upstream_bytecode_semantics=enabled\n"
      << "upstream_bytecode_semantics_exports=venom_qjs_upstream_bytecode_semantics_record_ptr|venom_qjs_upstream_bytecode_semantics_score\n"
      << "upstream_intrinsic_semantics=enabled\n"
      << "upstream_intrinsic_semantics_exports=venom_qjs_upstream_intrinsic_record_ptr|venom_qjs_upstream_property_descriptor_record_ptr|venom_qjs_upstream_prototype_chain_record_ptr|venom_qjs_upstream_call_frame_record_ptr|venom_qjs_upstream_intrinsic_semantics_score\n"
      << "semantic_record_export=venom_qjs_interpreter_semantic_record_ptr|venom_qjs_interpreter_semantic_record_size\n"
      << "global_slot_record_export=venom_qjs_global_slot_record_ptr|venom_qjs_global_slot_record_size\n"
      << "chunk_count=" << protected_count << "\n";
  for (const auto& chunk : bridge.chunks) {
    if ((chunk.flags & JsChunkBrowser) != 0u) continue;
    out << "wasm_chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk) << "\tflags=" << js_flags_summary(chunk.flags)
        << "\tbackend=" << (wasm_real ? "quickjs-wasm-real" : policy.backend) << "\n";
  }
  return out.str();
}

std::string make_quickjs_source_transfer_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_SOURCE_TRANSFER_V7\n"
      << "version=7\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "source_ptr=venom_qjs_script_buffer_ptr\n"
      << "source_capacity=venom_qjs_script_buffer_capacity\n"
      << "execute_source=venom_qjs_execute_source\n"
      << "execute_bytecode=venom_qjs_execute_bytecode\n"
      << "validate_bytecode=venom_qjs_bytecode_validate\n"
      << "bytecode_status=venom_qjs_status_code\n"
      << "bytecode_record_hash=venom_qjs_bytecode_record_hash32\n"
      << "bytecode_payload_size=venom_qjs_bytecode_payload_size\n"
      << "transfer_mode=opaque-vqjsbc03-native-object\n"
      << "oversized_record_threshold=786432\n"
      << "oversized_record_path=wasm-native-object-read\n"
      << "oversized_execution_export=venom_qjs_execute_bytecode\n"
      << "host_source_eval=false\n"
      << "protected_source_execution=false\n"
      << "interpreter_dispatch=venom_qjs_interpreter_ready|venom_qjs_interpreter_dispatch_count\n"
      << "result_ptr=venom_qjs_result_ptr\n"
      << "result_size=venom_qjs_result_size\n"
      << "encoding=native-quickjs-object-v3\n"
      << "host_call=quickjs.sourceTransfer\n";
  return out.str();
}
std::string make_quickjs_console_bridge_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_CONSOLE_BRIDGE_V4\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "mode=host-console-forward\n"
      << "host_call=quickjs.consoleBridge\n"
      << "levels=debug,log,info,warn,error\n"
      << "callback_abi=venom_qjs_console_callback_abi\n"
      << "event_ptr=venom_qjs_console_event_ptr\n"
      << "event_size=venom_qjs_console_event_size\n"
      << "event_count=venom_qjs_console_event_count\n";
  return out.str();
}

std::string make_script_engine_policy_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const bool allow_function_constructor = profile.kind == ProfileKind::Debug;
  std::ostringstream out;
  out << "VENOM_SCRIPT_ENGINE_POLICY_V4\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "fallback=deny-host-js-source-eval\n"
      << "abi_contract=quickjs-wasm-abi12-runtime\n"
      << "status_exports=venom_qjs_status_code,venom_qjs_status_record_ptr,venom_qjs_status_record_size,venom_qjs_release_status\n"
      << "limit_exports=venom_qjs_context_heap_limit,venom_qjs_context_heap_used,venom_qjs_context_stack_limit\n"
      << "bytecode_validation_exports=venom_qjs_bytecode_validate,venom_qjs_bytecode_record_hash32,venom_qjs_bytecode_payload_size,venom_qjs_bytecode_expected_source_hash32\n"
      << "allow_eval=false\n"
      << "allow_function_constructor=" << (allow_function_constructor ? "true" : "false") << "\n"
      << "capabilities=console,document,window,fetch,timers,events\n"
      << "bind_console=true\n"
      << "bind_document=true\n"
      << "bind_fetch=true\n"
      << "bind_timers=true\n"
      << "bind_events=true\n"
      << "route_scope=true\n"
      << "context_reuse=true\n"
      << "context_destroy=true\n"
      << "host_capability_injection=true\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "policy_chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tcapabilities=console,document,window,fetch,timers,events\n";
  }
  return out.str();
}

std::string make_quickjs_execution_journal_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::execution_journal_contract_text(venom::quickjs::kRuntimeAbiVersion,
                                                         venom::quickjs::kRuntimePackageVersion,
                                                         bridge.chunks.size(),
                                                         strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  for (const auto& chunk : bridge.chunks) {
    std::vector<unsigned char> source_bytes(chunk.code.begin(), chunk.code.end());
    out << "journal_seed\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk)
        << "\tsource_hash=0x" << std::hex << protected_source_hash(profile, chunk) << std::dec
        << "\tcheckpoint=prepare\n";
  }
  return out.str();
}

std::string make_quickjs_checkpoint_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::checkpoint_policy_contract_text(venom::quickjs::kRuntimeAbiVersion,
                                                         venom::quickjs::kRuntimePackageVersion,
                                                         strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_replay_cursor_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::replay_cursor_contract_text(venom::quickjs::kRuntimeAbiVersion,
                                                     venom::quickjs::kRuntimePackageVersion,
                                                     strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_resume_state_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::resume_state_contract_text(venom::quickjs::kRuntimeAbiVersion,
                                                    venom::quickjs::kRuntimePackageVersion,
                                                    strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_determinism_audit_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::determinism_audit_contract_text(venom::quickjs::kRuntimeAbiVersion,
                                                        venom::quickjs::kRuntimePackageVersion,
                                                        strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_snapshot_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::snapshot_policy_contract_text(venom::quickjs::kRuntimeAbiVersion,
                                                       venom::quickjs::kRuntimePackageVersion,
                                                       strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_snapshot_records_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::snapshot_records_contract_text(venom::quickjs::kRuntimeAbiVersion,
                                                        venom::quickjs::kRuntimePackageVersion,
                                                        bridge.chunks.size(),
                                                        strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  for (const auto& chunk : bridge.chunks) {
    std::vector<unsigned char> source_bytes(chunk.code.begin(), chunk.code.end());
    out << "snapshot_seed\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk)
        << "\tsource_hash=0x" << std::hex << protected_source_hash(profile, chunk) << std::dec
        << "\tstage=post-evaluate\n";
  }
  return out.str();
}

std::string make_quickjs_replay_validation_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::replay_validation_contract_text(venom::quickjs::kRuntimeAbiVersion,
                                                         venom::quickjs::kRuntimePackageVersion,
                                                         strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_determinism_ledger_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::determinism_ledger_contract_text(venom::quickjs::kRuntimeAbiVersion,
                                                          venom::quickjs::kRuntimePackageVersion,
                                                          strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_audit_seal_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::audit_seal_contract_text(venom::quickjs::kRuntimeAbiVersion,
                                                  venom::quickjs::kRuntimePackageVersion,
                                                  strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_execution_commit_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::execution_commit_contract_text(venom::quickjs::kRuntimeAbiVersion, venom::quickjs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_rollback_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::rollback_policy_contract_text(venom::quickjs::kRuntimeAbiVersion, venom::quickjs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_host_call_receipts_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::host_call_receipts_contract_text(venom::quickjs::kRuntimeAbiVersion, venom::quickjs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_release_acceptance_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::release_acceptance_contract_text(venom::quickjs::kRuntimeAbiVersion, venom::quickjs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_commit_audit_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::commit_audit_contract_text(venom::quickjs::kRuntimeAbiVersion, venom::quickjs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_capability_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::capability_policy_contract_text(venom::quickjs::kRuntimeAbiVersion, venom::quickjs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_host_io_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::host_io_policy_contract_text(venom::quickjs::kRuntimeAbiVersion, venom::quickjs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_permission_seal_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::permission_seal_contract_text(venom::quickjs::kRuntimeAbiVersion, venom::quickjs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_policy_receipts_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::policy_receipts_contract_text(venom::quickjs::kRuntimeAbiVersion, venom::quickjs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_release_gate_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::release_gate_contract_text(venom::quickjs::kRuntimeAbiVersion, venom::quickjs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_host_io_decision_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::host_io_decision_contract_text(venom::quickjs::kRuntimeAbiVersion, venom::quickjs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_host_io_deny_trace_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::host_io_deny_trace_contract_text(venom::quickjs::kRuntimeAbiVersion, venom::quickjs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_capability_ledger_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::capability_ledger_contract_text(venom::quickjs::kRuntimeAbiVersion, venom::quickjs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_policy_seal_audit_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::policy_seal_audit_contract_text(venom::quickjs::kRuntimeAbiVersion, venom::quickjs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_runtime_denylist_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::runtime_denylist_contract_text(venom::quickjs::kRuntimeAbiVersion, venom::quickjs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_context_lifecycle_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_CONTEXT_LIFECYCLE_V3\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "model=route-scoped-reusable\n"
      << "key=route|source|order\n"
      << "create_host_call=quickjs.contextCreate\n"
      << "reuse_host_call=quickjs.contextReuse\n"
      << "destroy_host_call=quickjs.contextDestroy\n"
      << "module_create_host_call=quickjs.moduleContextCreate\n"
      << "module_destroy_host_call=quickjs.moduleContextDestroy\n"
      << "snapshot=true\n"
      << "max_contexts=" << venom::quickjs::kDefaultContextLimit << "\n"
      << "heap_limit=quickjs-heap-limits.vqhl\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "context\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\treuse=true\tdestroy=route-unload\n";
  }
  return out.str();
}

std::string make_host_capabilities_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  out << "VENOM_HOST_CAPABILITIES_V2\n"
      << "version=2\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "inject_host_call=quickjs.hostCapabilityInject\n"
      << "capability\tconsole\treadonly\n"
      << "capability\tdocument\tbridge\n"
      << "capability\twindow\tbridge\n"
      << "capability\tnavigator\tguarded-browser-api-shim\n"
      << "capability\tfetch\tasync-host-call\n"
      << "capability\ttimers\tasync-host-call\n"
      << "capability\tevents\tqueue\n"
      << "capability\t__venomRuntime\tfrozen-bridge\n"
      << "default_capability_count=8\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "chunk_capabilities\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tconsole,document,window,navigator,fetch,timers,events,__venomRuntime\n";
  }
  return out.str();
}

std::string make_quickjs_adapter_diagnostics_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge, const std::string& engine_asset_name) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_ADAPTER_DIAGNOSTICS_V3\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "engine_asset=" << engine_asset_name << "\n"
      << "adapter_contract=createContext|executeChunk|destroyContext|contextSnapshot|status|parityProbe|abiTable\n"
      << "fallback=deny-host-js-source-eval\n"
      << "abi_contract=quickjs-wasm-abi12-runtime\n"
      << "status_exports=venom_qjs_status_code,venom_qjs_status_record_ptr,venom_qjs_status_record_size,venom_qjs_release_status\n"
      << "limit_exports=venom_qjs_context_heap_limit,venom_qjs_context_heap_used,venom_qjs_context_stack_limit\n"
      << "bytecode_validation_exports=venom_qjs_bytecode_validate,venom_qjs_bytecode_record_hash32,venom_qjs_bytecode_payload_size,venom_qjs_bytecode_expected_source_hash32\n"
      << "records=engineBoot,abiTable,contextCreate,contextSetLimits,scriptBufferAlloc,consoleCallbackAbi,hostImportTable,parityProbe,contextReuse,hostCapabilityInject,executeChunk,executionResult,contextDestroy,adapterStatus\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "adapter_chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk) << "\tflags=" << js_flags_summary(chunk.flags) << "\n";
  }
  return out.str();
}


std::string make_quickjs_execution_records_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_EXECUTION_RECORDS_V3\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "record_host_call=quickjs.executionRecord\n"
      << "snapshot_host_call=quickjs.executionSnapshot\n"
      << "record_fields=context,route,source,order,engine,wasmAccepted,fallbackUsed,consoleEvents,resultHash,abi,heapUsed,heapLimit,scriptBufferBytes,parityProbe\n"
      << "retention=runtime-session\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "execution_record\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tengine=quickjs-wasm-backend-candidate\tfallback=policy-gated\n";
  }
  return out.str();
}

std::string make_quickjs_result_bridge_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_RESULT_BRIDGE_V3\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "result_host_call=quickjs.resultBridge\n"
      << "wasm_decode_host_call=quickjs.wasmResultDecode\n"
      << "console_event_host_call=quickjs.consoleEvent\n"
      << "console_flush_host_call=quickjs.consoleFlush\n"
      << "format=json-record-v1\n"
      << "fields=ok,engine,version,abi,context,sourceBytes,sourceHash,consoleCount,fallbackRequired,heapUsed,heapLimit,scriptBufferBytes,parityProbe\n";
  return out.str();
}

std::string make_quickjs_fallback_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << "VENOM_QUICKJS_FALLBACK_POLICY_V3\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "mode=explicit-policy-gated\n"
      << "decision_host_call=script.fallbackDecision\n"
      << "policy_check_host_call=quickjs.fallbackPolicyCheck\n"
      << "allow_when=" << (strict ? "empty-chunk|real-engine-ready" : "engine-unavailable|wasm-interpreter-unavailable|empty-chunk") << "\n"
      << "deny_when=" << (strict ? "release-strict-no-fallback|engine-module-unavailable-or-compatible-fallback|wasm-interpreter-unavailable" : "release-strict-no-fallback") << "\n"
      << "current_release_policy=" << (strict ? "deny-host-fallback-unless-real-engine-ready" : "allow-compatible-fallback-with-record") << "\n"
      << "strict_release=" << (strict ? "true" : "false") << "\n"
      << "require_parity_probe=true\n"
      << "audit_records=quickjs-execution-records.vqer\n";
  return out.str();
}


std::string make_quickjs_engine_backend_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_ENGINE_BACKEND_V4\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "selected_backend=quickjs-wasm-real\n"
      << "native_backend=vendored-quickjs-c\n"
      << "browser_backend=quickjs-runtime-wasm-upstream-global-host-api-shims\n"
      << "scaffold_status=replaced-by-upstream-global-host-api-shims\n"
      << "replacement_path=active-upstream-global-host-api-shims-bridge\n"
      << "host_js_fallback_allowed=false\n"
      << "wasm_execution_contract=quickjs-wasm-execution.vqwe\n"
      << "backend_select_host_call=quickjs.backendSelect\n"
      << "real_engine_candidate_host_call=quickjs.realEngineCandidate\n"
      << "fallback_decision_host_call=quickjs.interpreterFallbackDecision\n"
      << "bytecode_chunks=route-scoped-script-chunk.*.vjsb\n"
      << "host_capabilities=host-capabilities.vhcap\n"
      << "native_parity=quickjs-native-parity.vqnp\n"
      << "execution_mode=quickjs-execution-mode.vqxm\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "backend_chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbackend=quickjs-wasm-real\tfallback=deny-host-js\n";
  }
  return out.str();
}

std::string make_quickjs_native_parity_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge, const std::string& probe) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_NATIVE_PARITY_V3\n"
      << "version=3\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "native_probe=" << probe << "\n"
      << "native_engine=vendored-quickjs-c\n"
      << "browser_engine=quickjs-runtime-wasm-upstream-global-host-api-shims\n"
      << "upstream_quickjs_source=third_party/quickjs\n"
      << "upstream_bridge=enabled\n"
      << "upstream_interpreter_core=quickjs-upstream-global-host-api-shims-v7\n"
      << "upstream_parity_record=venom_qjs_upstream_parity_record_ptr|venom_qjs_upstream_parity_record_size|venom_qjs_upstream_parity_record_hash\n"
      << "upstream_runtime_records=venom_qjs_upstream_object_record_ptr|venom_qjs_upstream_exception_record_ptr|venom_qjs_upstream_module_record_ptr|venom_qjs_upstream_runtime_bridge_score\n"
      << "upstream_bytecode_semantics=enabled\n"
      << "upstream_bytecode_semantics_records=venom_qjs_upstream_bytecode_semantics_record_ptr|venom_qjs_upstream_lexical_scope_record_ptr|venom_qjs_upstream_closure_record_ptr|venom_qjs_upstream_iterator_record_ptr|venom_qjs_upstream_bytecode_semantics_score\n"
      << "upstream_intrinsic_semantics=enabled\n"
      << "upstream_intrinsic_semantics_records=venom_qjs_upstream_intrinsic_record_ptr|venom_qjs_upstream_property_descriptor_record_ptr|venom_qjs_upstream_prototype_chain_record_ptr|venom_qjs_upstream_call_frame_record_ptr|venom_qjs_upstream_intrinsic_semantics_score\n"
      << "upstream_feature_exports=venom_qjs_upstream_feature_count|venom_qjs_upstream_builtin_count|venom_qjs_upstream_object_model_score|venom_qjs_upstream_exception_model_score|venom_qjs_upstream_module_model_score\n"
      << "parity_scope=context-create|bytecode-transfer|result-record|console-count|fallback-denied|global-slot-record|builtin-probe|object-model-score|module-model-score|intrinsic-semantics|property-descriptor|prototype-chain|call-frame\n"
      << "full_ecmascript_parity=false\n"
      << "full_bytecode_execution=upstream-global-host-api-shims-boundary\n"
      << "cutover_status=upstream-intrinsic-semantics-adapter-active\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "parity_chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk) << "\tflags=" << js_flags_summary(chunk.flags)
        << "\tupstream_bridge=quickjs-upstream-global-host-api-shims-v7\n";
  }
  return out.str();
}

std::string make_quickjs_execution_mode_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_EXECUTION_MODE_V3\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "mode=quickjs-wasm-real\n"
      << "fallback=fail-closed-no-host-js\n"
      << "deny_silent_fallback=true\n"
      << "record_backend_selection=true\n"
      << "require_source_transfer=true\n"
      << "require_result_bridge=true\n"
      << "require_console_bridge=true\n"
      << "release_policy=deny-host-fallback-require-wasm-backend\n";
  return out.str();
}


std::string make_quickjs_runtime_abi_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << venom::quickjs::runtime_abi_table_text()
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "abi_hash=0x" << std::hex << venom::quickjs::abi_table_hash() << std::dec << "\n"
      << "metadata_section=quickjs-runtime-abi.vqra\n";
  return out.str();
}

std::string make_quickjs_host_imports_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << venom::quickjs::host_import_table_text()
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "table_hash=0x" << std::hex << venom::quickjs::host_import_table_hash() << std::dec << "\n"
      << "host_call_import_table=quickjs-host-imports.vqhi\n";
  return out.str();
}

std::string make_quickjs_heap_limits_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_QJS_CONTEXT_LIMITS_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "default_heap_limit=" << venom::quickjs::kDefaultHeapLimitBytes << "\n"
      << "default_stack_limit=" << venom::quickjs::kDefaultStackLimitBytes << "\n"
      << "max_contexts=" << venom::quickjs::kDefaultContextLimit << "\n"
      << "set_limits_export=venom_qjs_context_set_limits\n"
      << "heap_limit_export=venom_qjs_context_heap_limit\n"
      << "heap_used_export=venom_qjs_context_heap_used\n"
      << "stack_limit_export=venom_qjs_context_stack_limit\n"
      << "release_fail_closed=" << (profile.fail_closed ? "true" : "false") << "\n";
  return out.str();
}

std::string make_quickjs_script_buffer_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_QJS_SCRIPT_BUFFER_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "encoding=utf-8\n"
      << "max_script_bytes=" << venom::quickjs::kDefaultScriptBufferLimitBytes << "\n"
      << "alloc_export=venom_qjs_script_buffer_alloc\n"
      << "ptr_export=venom_qjs_script_buffer_ptr\n"
      << "size_export=venom_qjs_script_buffer_size\n"
      << "capacity_export=venom_qjs_script_buffer_capacity\n"
      << "free_export=venom_qjs_script_buffer_free\n"
      << "legacy_source_ptr_export=venom_qjs_source_ptr\n";
  return out.str();
}

std::string make_quickjs_console_abi_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_QJS_CONSOLE_CALLBACK_ABI_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "callback_abi_export=venom_qjs_console_callback_abi\n"
      << "event_ptr_export=venom_qjs_console_event_ptr\n"
      << "event_size_export=venom_qjs_console_event_size\n"
      << "event_count_export=venom_qjs_console_event_count\n"
      << "event_clear_export=venom_qjs_console_clear\n"
      << "event_schema=level|route|source|order|message\n"
      << "host_call=quickjs.consoleCallbackAbi\n";
  return out.str();
}

std::string make_quickjs_parity_probe_metadata(const Profile& profile, const std::string& runtime_mode, const std::string& native_eval) {
  venom::quickjs::ParityProbe probe;
  probe.abi_hash = venom::quickjs::abi_table_hash();
  probe.host_import_hash = venom::quickjs::host_import_table_hash();
  probe.native_eval = native_eval;
  std::ostringstream out;
  out << venom::quickjs::parity_probe_text(probe)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "native_engine=vendored-quickjs-c\n"
      << "wasm_engine=quickjs-runtime.wasm\n"
      << "host_call=quickjs.wasmParityProbe\n";
  return out.str();
}

std::string make_quickjs_release_fallback_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << "VENOM_QJS_RELEASE_FALLBACK_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "strict_release=" << (strict ? "true" : "false") << "\n"
      << "deny_silent_fallback=true\n"
      << "allow_host_fallback=false\n"
      << "require_backend_record=true\n"
      << "require_parity_probe=true\n"
      << "host_call=quickjs.releaseFallbackDeny\n"
      << "policy=deny-host-fallback-require-wasm-backend\n";
  return out.str();
}

std::vector<venom::quickjs::BytecodeChunkRecord> make_quickjs_bytecode_records(const JsBridge& bridge, bool redact_metadata) {
  std::vector<venom::quickjs::BytecodeChunkRecord> records;
  std::vector<venom::quickjs::ModuleSourceRecord> module_sources;
  records.reserve(bridge.chunks.size());
  module_sources.reserve(bridge.chunks.size());
  for (const auto& chunk : bridge.chunks) {
    if ((chunk.flags & JsChunkBrowser) != 0u || (chunk.flags & JsChunkModule) == 0u) continue;
    const std::string compile_name = redact_metadata
        ? ("s_" + short_hash_hex(venom::package::fnv1a64(bytes_from_string("source|" + chunk.route + "|" + chunk.source + "|" + std::to_string(chunk.order))), 16))
        : chunk.source;
    module_sources.push_back({chunk.source, compile_name, chunk.code});
  }
  for (const auto& chunk : bridge.chunks) {
    if ((chunk.flags & JsChunkBrowser) != 0u) continue;
    const auto limit = static_cast<std::size_t>(venom::quickjs::kDefaultScriptBufferLimitBytes);
    if (chunk.code.size() > limit) {
      throw std::runtime_error("QuickJS script exceeds protected transfer limit: " + chunk.source +
                               " (" + std::to_string(chunk.code.size()) + " bytes > " +
                               std::to_string(limit) + " bytes)");
    }
    const bool is_module = (chunk.flags & JsChunkModule) != 0u;
    const std::string record_source = redact_metadata ? ("s_" + short_hash_hex(venom::package::fnv1a64(bytes_from_string("source|" + chunk.route + "|" + chunk.source + "|" + std::to_string(chunk.order))), 16)) : chunk.source;
    const auto bytecode = venom::quickjs::compile_native_quickjs_bytecode(
        chunk.code,
        record_source,
        is_module,
        redact_metadata,
        is_module ? &module_sources : nullptr);
    if (bytecode.size() > limit) {
      throw std::runtime_error("QuickJS bytecode exceeds protected transfer limit: " + chunk.source +
                               " (" + std::to_string(bytecode.size()) + " bytes > " +
                               std::to_string(limit) + " bytes)");
    }
    std::vector<unsigned char> source_bytes(chunk.code.begin(), chunk.code.end());
    const std::string record_route = redact_metadata ? ("r_" + short_hash_hex(venom::package::fnv1a64(bytes_from_string("route|" + chunk.route)), 16)) : chunk.route;
    records.push_back({chunk.order,
                       record_route,
                       record_source,
                       redact_metadata ? 0u : static_cast<std::uint32_t>(chunk.code.size()),
                       redact_metadata ? 0u : venom::package::fnv1a64(source_bytes),
                       static_cast<std::uint32_t>(bytecode.size()),
                       venom::quickjs::BytecodeFormat::NativeQuickJsObjectV3});
  }
  return records;
}

std::string make_quickjs_bytecode_manifest_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const bool redact_metadata = profile.kind == ProfileKind::Release || profile.strip_debug_metadata;
  auto records = make_quickjs_bytecode_records(bridge, redact_metadata);
  std::ostringstream out;
  out << venom::quickjs::bytecode_manifest_text(records, venom::quickjs::kRuntimeAbiVersion, venom::quickjs::kRuntimePackageVersion)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "section=quickjs-bytecode-manifest.vqbm\n";
  return out.str();
}

std::string make_quickjs_module_resolver_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  out << venom::quickjs::module_resolver_metadata_text(venom::quickjs::kRuntimeAbiVersion,
                                                       venom::quickjs::kRuntimePackageVersion,
                                                       bridge.chunks.size())
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_exception_abi_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << venom::quickjs::exception_abi_metadata_text(venom::quickjs::kRuntimeAbiVersion,
                                                     venom::quickjs::kRuntimePackageVersion)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_host_trap_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::host_trap_policy_metadata_text(strict,
                                                        venom::quickjs::kRuntimeAbiVersion,
                                                        venom::quickjs::kRuntimePackageVersion)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_execution_lifecycle_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::execution_lifecycle_text(strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_script_buffer_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::script_buffer_policy_text(venom::quickjs::kDefaultScriptBufferLimitBytes, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_context_limit_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::context_limit_policy_text(venom::quickjs::kDefaultHeapLimitBytes,
                                                   venom::quickjs::kDefaultStackLimitBytes,
                                                   venom::quickjs::kDefaultContextLimit,
                                                   venom::quickjs::kDefaultScriptBufferLimitBytes,
                                                   venom::quickjs::kDefaultHostCallLimit,
                                                   venom::quickjs::kDefaultConsoleEventLimit,
                                                   venom::quickjs::kDefaultModuleRecordLimit,
                                                   strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_host_call_dispatch_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::host_call_dispatch_table_text(strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "dispatch_hash=0x" << std::hex << venom::quickjs::host_call_dispatch_table_hash(strict) << std::dec << "\n";
  return out.str();
}

std::string make_quickjs_parity_contract_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  const auto dispatch_hash = venom::quickjs::host_call_dispatch_table_hash(strict);
  std::ostringstream out;
  out << venom::quickjs::parity_contract_text(venom::quickjs::kRuntimeAbiVersion,
                                              venom::quickjs::kRuntimePackageVersion,
                                              venom::quickjs::abi_table_hash(),
                                              venom::quickjs::host_import_table_hash(),
                                              dispatch_hash,
                                              strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_release_failclosed_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::release_failclosed_policy_text(strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}


std::size_t quickjs_module_count(const JsBridge& bridge) {
  return std::count_if(bridge.chunks.begin(), bridge.chunks.end(), [](const JsChunk& chunk) {
    return (chunk.flags & JsChunkModule) != 0u;
  });
}

std::string make_quickjs_module_graph_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  const auto module_count = quickjs_module_count(bridge);
  const bool redact_metadata = profile.kind == ProfileKind::Release || profile.strip_debug_metadata;
  out << "VENOM_QJS_MODULE_GRAPH_V1\n"
      << "version=1\n"
      << "runtime_abi=" << venom::quickjs::kRuntimeAbiVersion << "\n"
      << "package_version=" << venom::quickjs::kRuntimePackageVersion << "\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "graph_format=route-scoped-script-module-records\n"
      << "chunk_count=" << bridge.chunks.size() << "\n"
      << "module_count=" << module_count << "\n"
      << "resolver=package-relative-first\n"
      << "dynamic_import=protected-module-bundle-vqjsmb04\n"
      << "edge_count=" << bridge.module_edges.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    std::vector<unsigned char> source_bytes(chunk.code.begin(), chunk.code.end());
    const std::string route_label = redact_metadata
        ? ("r_" + short_hash_hex(venom::package::fnv1a64(bytes_from_string("route|" + chunk.route)), 16))
        : chunk.route;
    const std::string source_label = redact_metadata
        ? ("s_" + short_hash_hex(venom::package::fnv1a64(bytes_from_string("source|" + chunk.route + "|" + chunk.source + "|" + std::to_string(chunk.order))), 16))
        : chunk.source;
    out << "module\t" << chunk.order << "\t" << route_label << "\t" << source_label
        << "\tbytes=" << (redact_metadata ? 0u : chunk.code.size())
        << "\thash=0x" << std::hex << (redact_metadata ? 0u : venom::package::fnv1a64(source_bytes)) << std::dec
        << "\tflags=" << js_flags_summary(chunk.flags)
        << "\tis_module=" << (((chunk.flags & JsChunkModule) != 0u) ? "true" : "false") << "\n";
  }
  for (const auto& edge : bridge.module_edges) {
    const std::string referrer = redact_metadata ? ("s_" + short_hash_hex(venom::package::fnv1a64(bytes_from_string("referrer|" + edge.referrer)), 16)) : edge.referrer;
    const std::string resolved = redact_metadata ? ("s_" + short_hash_hex(venom::package::fnv1a64(bytes_from_string("resolved|" + edge.resolved)), 16)) : edge.resolved;
    out << "edge\t" << (edge.dynamic ? "dynamic" : "static") << "\t" << referrer << "\t" << resolved << "\n";
  }
  return out.str();
}

std::string make_quickjs_module_execution_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::module_execution_contract_text(venom::quickjs::kRuntimeAbiVersion,
                                                        venom::quickjs::kRuntimePackageVersion,
                                                        quickjs_module_count(bridge),
                                                        strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_module_cache_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  out << venom::quickjs::module_cache_policy_text(venom::quickjs::kRuntimeAbiVersion,
                                                  venom::quickjs::kRuntimePackageVersion,
                                                  quickjs_module_count(bridge))
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_resolver_audit_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::resolver_audit_policy_text(venom::quickjs::kRuntimeAbiVersion,
                                                    venom::quickjs::kRuntimePackageVersion,
                                                    quickjs_module_count(bridge),
                                                    strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_interop_fallback_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::interop_fallback_policy_text(venom::quickjs::kRuntimeAbiVersion,
                                                      venom::quickjs::kRuntimePackageVersion,
                                                      strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_execution_pipeline_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::execution_pipeline_contract_text(venom::quickjs::kRuntimeAbiVersion,
                                                          venom::quickjs::kRuntimePackageVersion,
                                                          bridge.chunks.size(),
                                                          strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_script_records_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::script_records_contract_text(venom::quickjs::kRuntimeAbiVersion,
                                                      venom::quickjs::kRuntimePackageVersion,
                                                      bridge.chunks.size(),
                                                      strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  for (const auto& chunk : bridge.chunks) {
    std::vector<unsigned char> source_bytes(chunk.code.begin(), chunk.code.end());
    out << "script_record\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk)
        << "\thash=0x" << std::hex << protected_source_hash(profile, chunk) << std::dec
        << "\tflags=" << js_flags_summary(chunk.flags) << "\n";
  }
  return out.str();
}

std::string make_quickjs_eval_results_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::eval_results_contract_text(venom::quickjs::kRuntimeAbiVersion,
                                                    venom::quickjs::kRuntimePackageVersion,
                                                    strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_console_capture_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::console_capture_contract_text(venom::quickjs::kRuntimeAbiVersion,
                                                       venom::quickjs::kRuntimePackageVersion,
                                                       strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_quickjs_failure_reports_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Release;
  std::ostringstream out;
  out << venom::quickjs::failure_reports_contract_text(venom::quickjs::kRuntimeAbiVersion,
                                                       venom::quickjs::kRuntimePackageVersion,
                                                       strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_runtime_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  if (profile.kind == ProfileKind::Release) {
    // Compact authenticated release policy. The fixed-width format avoids text
    // decoding, splitting, and string-key lookups in the protected loader.
    // Layout: magic[8]="VRPOL002", version:u32, flags:u32, profile_hash:u32,
    // runtime_hash:u32. All integer fields are little-endian.
    std::string out("VRPOL002", 8);
    auto append_u32 = [&out](std::uint32_t value) {
      out.push_back(static_cast<char>(value & 0xffu));
      out.push_back(static_cast<char>((value >> 8u) & 0xffu));
      out.push_back(static_cast<char>((value >> 16u) & 0xffu));
      out.push_back(static_cast<char>((value >> 24u) & 0xffu));
    };
    auto fnv32 = [](const std::string& text) {
      std::uint32_t h = 2166136261u;
      for (unsigned char c : text) { h ^= c; h *= 16777619u; }
      return h;
    };
    std::uint32_t flags = 0u;
    if (profile.fail_closed) flags |= 1u << 0u;
    if (profile.runtime_hardened) flags |= 1u << 1u;
    if (profile.strip_debug_metadata) flags |= 1u << 2u;
    if (profile.runtime_hardened) flags |= 1u << 3u; // freeze host bridge
    if (profile.integrity_metadata) flags |= 1u << 4u;
    if (runtime_mode == "wasm") flags |= 1u << 5u;
    flags |= 1u << 6u; // safe failure UI
    flags |= 1u << 7u; // fetch bridge
    flags |= 1u << 8u; // timer bridge
    flags |= 1u << 9u; // event queue
    flags |= 1u << 10u; // async host queue
    append_u32(2u);
    append_u32(flags);
    append_u32(fnv32(profile.name));
    append_u32(fnv32(runtime_mode));
    return out;
  }
  std::ostringstream out;
  out << "VENOM_RUNTIME_POLICY_V1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "wasm_runtime=" << (runtime_mode == "wasm" ? "true" : "false") << "\n"
      << "fail_closed=" << (profile.fail_closed ? "true" : "false") << "\n"
      << "runtime_hardened=" << (profile.runtime_hardened ? "true" : "false") << "\n"
      << "strip_debug_metadata=" << (profile.strip_debug_metadata ? "true" : "false") << "\n"
      << "freeze_host_bridge=" << (profile.runtime_hardened ? "true" : "false") << "\n"
      << "require_integrity=" << (profile.integrity_metadata ? "true" : "false") << "\n"
      << "package_hash_allowlist=loader\n"
      << "safe_failure_ui=true\n"
      << "binary_dom_ops=" << (runtime_mode == "wasm" ? "true" : "false") << "\n"
      << "fetch_bridge=true\n"
      << "timer_bridge=true\n"
      << "event_queue=true\n"
      << "async_host_queue=true\n"
      << "quickjs_bridge=engine-module\n"
      << "script_isolation=route-scoped\n"
      << "script_policy=capability-metadata\n"
      << "quickjs_chunks=engine-input\n"
      << "quickjs_engine=module-loader\n"
      << "quickjs_context_lifecycle=tracked\n"
      << "host_capabilities=injected\n"
      << "quickjs_adapter_diagnostics=enabled\n"
      << "script_engine_fallback=module-fallback-policy\n"
      << "quickjs_wasm_runtime=scaffold-asset\n"
      << "quickjs_source_transfer=wasm-memory\n"
      << "quickjs_console_bridge=host-console-forward\n"
      << "quickjs_execution_records=enabled\n"
      << "quickjs_result_bridge=enabled\n"
      << "quickjs_fallback_policy=enforced\n"
      << "quickjs_engine_backend=quickjs-wasm-backend-candidate\n"
      << "quickjs_native_parity=metadata\n"
      << "quickjs_execution_mode=prefer-backend-candidate\n"
      << "quickjs_runtime_abi=table-v4\n"
      << "quickjs_heap_limits=context-accounting\n"
      << "quickjs_script_buffer=alloc-api\n"
      << "quickjs_console_callback_abi=enabled\n"
      << "quickjs_host_import_table=designed\n"
      << "quickjs_parity_probe=native-wasm\n"
      << "quickjs_release_fallback=" << ((profile.fail_closed || profile.kind == ProfileKind::Release) ? "strict-deny" : "policy-gated") << "\n"
      << "quickjs_bytecode_manifest=native-quickjs-object-bytecode-v3\n"
      << "quickjs_module_resolver=package-relative-map\n"
      << "quickjs_exception_abi=json-record-v1\n"
      << "quickjs_host_trap_policy=" << ((profile.fail_closed || profile.kind == ProfileKind::Release) ? "deny-unknown-host-imports" : "record-unknown-host-imports") << "\n"
      << "quickjs_execution_lifecycle=state-machine-v1\n"
      << "quickjs_script_buffer_policy=validate-hash-before-execute\n"
      << "quickjs_context_limit_policy=enforced\n"
      << "quickjs_host_call_dispatch=stable-id-table-v1\n"
      << "quickjs_parity_contract=abi-hash-limit-trap-host-table\n"
      << "quickjs_release_failclosed=enabled\n"
      << "quickjs_execution_pipeline=records-v1\n"
      << "quickjs_script_records=prepared-records-v1\n"
      << "quickjs_eval_results=eval-json-records-v1\n"
      << "quickjs_console_capture=capture-drain-v1\n"
      << "quickjs_failure_reports=failclosed-json-records-v1\n";
  return out.str();
}

bool should_encrypt_section(const Profile& profile, venom::package::SectionType type, const std::string& name) {
  if (!profile.crypto_provider_ready || profile.kind == ProfileKind::Debug) {
    return false;
  }
  if (type == venom::package::SectionType::Asset) {
    return false;
  }
  if (type == venom::package::SectionType::Css) {
    return false;
  }
  (void)name;
  return true;
}


std::size_t count_marker(const std::vector<unsigned char>& bytes, const std::string& marker) {
  if (marker.empty() || bytes.size() < marker.size()) {
    return 0;
  }
  std::size_t count = 0;
  auto it = bytes.begin();
  while ((it = std::search(it, bytes.end(), marker.begin(), marker.end())) != bytes.end()) {
    ++count;
    ++it;
  }
  return count;
}

void print_build_protection_report(const Profile& profile,
                                   const BuildOptions& options,
                                   const std::filesystem::path& package_path,
                                   const std::vector<unsigned char>& package_bytes,
                                   const venom::package::Package& package,
                                   bool external_manifest) {
  std::size_t encrypted_sections = 0;
  std::size_t compressed_sections = 0;
  bool manifest_section = false;
  bool package_binding = false;
  bool release_profile = false;
  bool layout_polymorphic = false;
  bool lazy_sections = false;
  std::size_t quickjs_bytecode_records = 0;
  for (const auto& section : package.sections) {
    if ((section.flags & venom::package::SectionFlagEncrypted) != 0u) {
      ++encrypted_sections;
    }
    if ((section.flags & venom::package::SectionFlagCompressed) != 0u) {
      ++compressed_sections;
    }
    if (section.type == venom::package::SectionType::Manifest && section.name == "manifest.txt") {
      manifest_section = true;
    }
    if (section.type == venom::package::SectionType::Integrity && venom::package::section_name_matches(section.type, section.name, "package-binding.vbind")) {
      package_binding = true;
    }
    if (section.type == venom::package::SectionType::Integrity && venom::package::section_name_matches(section.type, section.name, "release-profile.vrpf")) {
      release_profile = true;
    }
    if (section.type == venom::package::SectionType::Integrity && venom::package::section_name_matches(section.type, section.name, "package-layout.vlay")) {
      layout_polymorphic = true;
    }
    if (section.type == venom::package::SectionType::Integrity && venom::package::section_name_matches(section.type, section.name, "lazy-sections.vlazy")) {
      lazy_sections = true;
    }
    if (section.type == venom::package::SectionType::JavaScript) {
      quickjs_bytecode_records += count_marker(section.data, "VQJSBC03");
    }
  }
  const auto runtime_sections = count_marker(package_bytes, "VAEAD001");
  const auto sodium_sections = count_marker(package_bytes, "VSODIUM1");
  const auto legacy_sections = count_marker(package_bytes, "VSEAL001");
  const bool audited = sodium_sections != 0u;
  const bool browser_runnable = sodium_sections == 0u;
  const bool release_pass = profile.kind != ProfileKind::Debug &&
                            encrypted_sections != 0u &&
                            legacy_sections == 0u &&
                            !external_manifest &&
                            !manifest_section &&
                            package_binding &&
                            release_profile &&
                            (!options.require_audited_crypto || audited);
  std::cout << "Protection report:\n"
            << "  profile: " << profile.name << "\n"
            << "  target: " << options.security_target << "\n"
            << "  package: " << package_path.string() << "\n"
            << "  protected_sections: " << encrypted_sections << "\n"
            << "  compressed_sections: " << compressed_sections << "\n"
            << "  provider: " << selected_section_sealer_name(profile, options.crypto_provider) << "\n"
            << "  provider_runtime_sections: " << runtime_sections << "\n"
            << "  provider_libsodium_sections: " << sodium_sections << "\n"
            << "  provider_legacy_sections: " << legacy_sections << "\n"
            << "  quickjs_bytecode_records: " << quickjs_bytecode_records << "\n"
            << "  package_binding: " << (package_binding ? "yes" : "no") << "\n"
            << "  release_profile: " << (release_profile ? "yes" : "no") << "\n"
            << "  layout_polymorphic: " << (layout_polymorphic ? "yes" : "no") << "\n"
            << "  lazy_sections: " << (lazy_sections ? "yes" : "no") << "\n"
            << "  debug_metadata: " << (profile.strip_debug_metadata ? "stripped" : "present") << "\n"
            << "  external_debug_manifest: " << (external_manifest ? "yes" : "no") << "\n"
            << "  package_manifest_section: " << (manifest_section ? "yes" : "no") << "\n"
            << "  browser_executable: " << (browser_runnable ? "yes" : "no") << "\n"
            << "  native_private_crypto: " << (audited ? "yes" : "no") << "\n"
            << "  release_status: " << (release_pass ? "PASS" : "CHECK") << "\n";
}

bool should_ship_manifest_section(const Profile& profile, const BuildOptions& options) {
  return !profile.strip_debug_metadata || options.emit_debug_manifest;
}

bool should_emit_external_asset_manifest(const Profile& profile, const BuildOptions& options) {
  return profile.kind == ProfileKind::Debug || options.emit_debug_manifest;
}

void shuffle_package_sections(std::vector<PendingPackageSection>& sections, const venom::vm::PolymorphicPlan& poly) {
  if (!poly.enabled || sections.size() < 3) {
    return;
  }

  // Keep the manifest first for inspect readability, but randomize the rest.
  venom::vm::DiversificationRng rng(poly, "package-section-order");
  std::shuffle(sections.begin() + 1, sections.end(), rng);
}
} // namespace

bool build_site(const BuildOptions& requested_options) {
  BuildOptions production_options = requested_options;
  production_options.profile = "browser-protect";
  production_options.hashed_assets = true;
  production_options.package_mode = "external";
  production_options.runtime = "wasm";
  production_options.quickjs_backend = "wasm-real";
  production_options.crypto_provider = "runtime";
  production_options.allow_host_fallback = false;
  production_options.deny_host_fallback = true;
  production_options.strict_release = true;
  production_options.emit_debug_manifest = false;
  production_options.security_target = "browser";
  const BuildOptions& options = production_options;
  if (!options.key_file.empty()) {
    load_package_key_file_for_process(options.key_file);
  }
  if (options.require_audited_crypto && options.crypto_provider != "libsodium") {
    throw std::runtime_error("--require-audited-crypto requires --crypto-provider libsodium or --profile native-protect");
  }
  const auto profile = resolve_profile(options.profile);
  require_real_embedded_quickjs_runtime();
  const auto graph = collect_site(options.input);

  if (graph.routes.empty()) {
    throw std::runtime_error("site has no html routes");
  }

  const auto vendor_lock_path = options.vendor_lock.empty()
      ? (graph.root / "venom.lock")
      : std::filesystem::absolute(options.vendor_lock);
  const auto vendor_lock = load_remote_vendor_lock(vendor_lock_path);

  RemoteVendorOptions remote_vendor_options;
  remote_vendor_options.enabled = true;
  remote_vendor_options.offline = options.vendor_offline;
  remote_vendor_options.refresh = options.refresh_vendors;
  remote_vendor_options.cache_directory = options.vendor_cache.empty()
      ? (std::filesystem::absolute(options.output).parent_path() / ".venom-cache" / graph.root.filename() / "remote")
      : std::filesystem::absolute(options.vendor_cache);
  remote_vendor_options.lock_file = vendor_lock_path;
  remote_vendor_options.lock_present = vendor_lock.present;
  remote_vendor_options.lock_entries = vendor_lock.entries;
  {
    const char* reproducible_epoch = std::getenv("SOURCE_DATE_EPOCH");
    if (reproducible_epoch && *reproducible_epoch) {
      remote_vendor_options.bridge_id_salt = std::string{"epoch:"} + reproducible_epoch;
    } else {
      const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
      remote_vendor_options.bridge_id_salt = std::string{"nonce:"} + std::to_string(now);
    }
  }
  const auto js_bridge = compile_js_bridge(graph, remote_vendor_options);
  require_embedded_quickjs_module_bundle_runtime(js_bridge);
  if (options.vendor_offline && !vendor_lock.present && !js_bridge.remote_vendors.empty()) {
    throw std::runtime_error("offline remote vendoring requires venom.lock; run one reviewed online build first");
  }
  validate_remote_vendor_lock_coverage(vendor_lock, js_bridge.remote_vendors, options.refresh_vendors);
  // Validate every protected script/bytecode record before touching the output
  // directory. A capacity failure must never leave a partial production dist.
  (void)make_quickjs_bytecode_records(js_bridge, profile.kind == ProfileKind::Release || profile.strip_debug_metadata);
  const auto vendor_lock_digest = remote_vendor_lock_sha256(js_bridge.remote_vendors);
  const std::string vendor_lock_mode = options.refresh_vendors
      ? "refreshed"
      : (vendor_lock.present ? "enforced" : "generated");
  const auto release_policy = resolve_release_build_policy(profile, options, js_bridge.chunks.size());
  enforce_release_build_policy(release_policy, js_bridge.chunks.size());

  const bool hardened_release_asset = profile.fail_closed || profile.kind == ProfileKind::Release || options.strict_release;
  if (hardened_release_asset) {
    const auto bytecode_format = wasm_truth_value("bytecode_format", "");
    const auto native_reader = wasm_truth_value("native_object_reader", "");
    const auto source_materialization = wasm_truth_value("source_materialization", "");
    if (bytecode_format != "VQJSBC03" || native_reader != "JS_ReadObject" || source_materialization != "false") {
      throw std::runtime_error(
          "embedded QuickJS WASM is legacy or incompatible with native VQJSBC03 records; "
          "rebuild and embed it with scripts/build-quickjs-wasm before producing a protected release");
    }
  }

  std::filesystem::remove_all(options.output);
  const auto assets_dir = options.output / "assets";
  std::filesystem::create_directories(assets_dir);

  const auto assets = build_asset_pipeline(graph, options.hashed_assets);
  const auto css = bundle_css(graph, assets);
  const bool wasm_runtime = options.runtime == "wasm";
  if (options.runtime != "js" && options.runtime != "wasm") {
    throw std::runtime_error("unsupported runtime mode: " + options.runtime);
  }
  const auto poly = venom::vm::make_polymorphic_plan(profile.deterministic ? 0xC0FFEEu : 0u, profile.polymorphic);
  const auto capability_graph = analyze_capabilities(graph.root);
  const auto runtime_modules = plan_runtime_modules(graph, capability_graph);
  const auto wasm_bytes = wasm_runtime ? make_runtime_wasm_module() : std::vector<unsigned char>{};
  const auto wasm_file_name = wasm_runtime ? named_output(hardened_release_asset ? "rw" : "runtime", ".wasm", wasm_bytes, options.hashed_assets) : std::string{};
  const auto wasm_name = hardened_release_asset && wasm_runtime ? "runtime/" + wasm_file_name : wasm_file_name;
  // The bridge and its companion WASM live beside each other in protected output,
  // so the generated module uses the basename while package metadata records the
  // canonical distribution path.
  const auto wasm_runtime_asset_ref = hardened_release_asset ? wasm_name : wasm_file_name;
  auto runtime = wasm_runtime ? make_runtime_wasm_bridge_js(graph, wasm_runtime_asset_ref, hardened_release_asset, &poly) : make_runtime_js(graph, hardened_release_asset);
  runtime = specialize_runtime_modules(std::move(runtime), runtime_modules);
  const auto js_preview = js_bridge.preview.empty() ? bundle_js_preview(graph) : js_bridge.preview;
  const auto compiled_routes = compile_html_routes(graph, poly);

  venom::quickjs::Engine engine;
  const auto qjs_probe = engine.eval_string("'quickjs:' + (1 + 1)");

  const auto style_file_name = named_output(hardened_release_asset ? "s" : "style", ".css", css, options.hashed_assets);
  const auto style_name = hardened_release_asset ? "style/" + style_file_name : style_file_name;
  auto quickjs_engine_module = make_quickjs_engine_module_js(hardened_release_asset);
  auto quickjs_wasm_bytes = make_quickjs_runtime_wasm_module();
  const auto bridge_invoke_opcode = bridge_protocol_opcode(remote_vendor_options.bridge_id_salt, "invoke");
  const auto bridge_cancel_opcode = bridge_protocol_opcode(remote_vendor_options.bridge_id_salt, "cancel");
  const auto bridge_result_opcode = bridge_protocol_opcode(remote_vendor_options.bridge_id_salt, "result");
  const auto bridge_error_opcode = bridge_protocol_opcode(remote_vendor_options.bridge_id_salt, "error");
  auto worker_runtime = hardened_release_asset ? make_worker_runtime_js(js_bridge.bridge_candidate_ids, js_bridge.bridge_registry_bytecode,
      bridge_invoke_opcode, bridge_cancel_opcode, bridge_result_opcode, bridge_error_opcode) : std::string{};
  if (hardened_release_asset) {
    const auto wasm_export_aliases = compact_quickjs_wasm_exports(quickjs_wasm_bytes, remote_vendor_options.bridge_id_salt);
    redact_quickjs_wasm_abi_table(quickjs_wasm_bytes, remote_vendor_options.bridge_id_salt);
    apply_wasm_export_aliases(runtime, wasm_export_aliases);
    apply_wasm_export_aliases(quickjs_engine_module, wasm_export_aliases);
    apply_wasm_export_aliases(worker_runtime, wasm_export_aliases);
    redact_unmapped_quickjs_symbols(runtime, remote_vendor_options.bridge_id_salt);
    redact_unmapped_quickjs_symbols(quickjs_engine_module, remote_vendor_options.bridge_id_salt);
    redact_unmapped_quickjs_symbols(worker_runtime, remote_vendor_options.bridge_id_salt);
    runtime = harden_release_js_asset(std::move(runtime));
    runtime = ast_harden_release_js("runtime", runtime);
    validate_protected_js_asset("runtime", runtime);
    quickjs_engine_module = harden_release_js_asset(std::move(quickjs_engine_module));
    quickjs_engine_module = ast_harden_release_js("engine", quickjs_engine_module);
    validate_protected_js_asset("quickjs-engine", quickjs_engine_module);
  }
  if (!hardened_release_asset) {
    // Development output remains readable and uses the descriptive ABI names.
  }
  const auto runtime_file_name = named_output((hardened_release_asset && wasm_runtime) ? "r" : (wasm_runtime ? "runtime-js-bridge" : "runtime"), ".js", runtime, options.hashed_assets);
  const auto runtime_name = hardened_release_asset ? "runtime/" + runtime_file_name : runtime_file_name;
  const auto quickjs_engine_file_name = named_output(hardened_release_asset ? "engine" : "quickjs-engine", ".js", quickjs_engine_module, options.hashed_assets);
  const auto quickjs_engine_name = hardened_release_asset ? "runtime/" + quickjs_engine_file_name : quickjs_engine_file_name;
  const auto quickjs_wasm_file_name = named_output(hardened_release_asset ? "runtime" : "quickjs-runtime", ".wasm", quickjs_wasm_bytes, options.hashed_assets);
  const auto quickjs_wasm_name = hardened_release_asset ? "runtime/" + quickjs_wasm_file_name : quickjs_wasm_file_name;
  if (hardened_release_asset) {
    worker_runtime = harden_release_js_asset(std::move(worker_runtime));
    worker_runtime = ast_harden_release_js("worker", worker_runtime);
    validate_protected_js_asset("worker", worker_runtime);
  }
  const auto worker_file_name = hardened_release_asset ? named_output("worker", ".js", worker_runtime, options.hashed_assets) : std::string{};
  const auto worker_name = hardened_release_asset ? "workers/" + worker_file_name : std::string{};
  const auto package_binding_token = make_package_binding_token(profile.deterministic, profile.name + "|" + options.runtime + "|" + runtime_name + "|" + wasm_name + "|" + quickjs_engine_name + "|" + quickjs_wasm_name + "|" + worker_name);
  const auto package_flags = profile.package_flags |
      ((options.strict_release || profile.fail_closed) ? venom::package::PackageFlagReleaseProfile : 0u) |
      venom::package::PackageFlagTimerBridge |
      venom::package::PackageFlagEventQueue |
      venom::package::PackageFlagQuickJsBridge |
      venom::package::PackageFlagScriptIsolation |
      venom::package::PackageFlagScriptPolicy |
      venom::package::PackageFlagQuickJsChunks |
      venom::package::PackageFlagQuickJsEngine |
      venom::package::PackageFlagScriptEngineFallback |
      venom::package::PackageFlagQuickJsEngineModule |
      venom::package::PackageFlagQuickJsContextLifecycle |
      venom::package::PackageFlagHostCapabilities |
      venom::package::PackageFlagQuickJsAdapterDiagnostics |
      venom::package::PackageFlagQuickJsWasmRuntime |
      venom::package::PackageFlagQuickJsSourceTransfer |
      venom::package::PackageFlagQuickJsConsoleBridge |
      venom::package::PackageFlagQuickJsExecutionRecords |
      venom::package::PackageFlagQuickJsResultBridge |
      venom::package::PackageFlagQuickJsFallbackPolicy |
      venom::package::PackageFlagQuickJsEngineBackend |
      (wasm_runtime ? (venom::package::PackageFlagWasmRuntime | venom::package::PackageFlagBinaryDomOps) : 0u);

  std::vector<PendingPackageSection> package_sections;
  const auto add_package_section = [&](venom::package::SectionType type,
                                       std::string name,
                                       std::vector<unsigned char> data,
                                       std::uint32_t flags = venom::package::SectionFlagNone) {
    if (should_encrypt_section(profile, type, name)) {
      flags |= venom::package::SectionFlagEncrypted;
    }
    package_sections.push_back({type, flags, std::move(name), std::move(data)});
  };

  if (should_ship_manifest_section(profile, options)) {
    add_package_section(venom::package::SectionType::Manifest, "manifest.txt", bytes_from_string(html_manifest(graph, &compiled_routes, "asset-manifest.txt")));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::Integrity, "profile-diagnostics.txt", bytes_from_string(describe_profile(profile)));
  }
  add_package_section(venom::package::SectionType::Integrity, "runtime-policy.vhrd", bytes_from_string(make_runtime_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::Integrity, "package-binding.vbind", bytes_from_string(make_package_binding_metadata(profile, options, package_binding_token, runtime_name, runtime, wasm_name, wasm_bytes, style_name, css, quickjs_engine_name, quickjs_engine_module, quickjs_wasm_name, quickjs_wasm_bytes, worker_name, worker_runtime)));
  add_package_section(venom::package::SectionType::Integrity, "release-profile.vrpf", bytes_from_string(make_release_profile_metadata(profile, options, release_policy, js_bridge.chunks.size())));
  add_package_section(venom::package::SectionType::Integrity, "remote-vendors.vrvd", bytes_from_string(make_remote_vendor_metadata(js_bridge.remote_vendors, remote_vendor_options.offline, remote_vendor_options.cache_directory, vendor_lock_path, vendor_lock_digest, vendor_lock_mode)));
  add_package_section(venom::package::SectionType::QuickJsReleaseFailClosed, "quickjs-release-build-policy.vqbp", bytes_from_string(make_release_build_policy_metadata(profile, options, release_policy, js_bridge.chunks.size())));
  add_package_section(venom::package::SectionType::HostBridge, "host-calls.vhcb", bytes_from_string(make_host_bridge_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::FetchBridge, "fetch-bridge.vfch", bytes_from_string(make_fetch_bridge_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TimerBridge, "timer-bridge.vtmr", bytes_from_string(make_timer_bridge_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::EventQueue, "event-queue.vevq", bytes_from_string(make_event_queue_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsBridge, "quickjs-bridge.vqjs", bytes_from_string(make_quickjs_bridge_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::ScriptIsolation, "script-isolation.vsis", bytes_from_string(make_script_isolation_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::ScriptPolicy, "script-policy.vscp", bytes_from_string(make_script_policy_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsChunks, "quickjs-chunks.vqbc", bytes_from_string(make_quickjs_chunk_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsEngine, "quickjs-engine.vqse", bytes_from_string(make_quickjs_engine_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsEngineModule, "quickjs-engine-module.vqem", bytes_from_string(make_quickjs_engine_module_metadata(profile, options.runtime, js_bridge, quickjs_engine_name, quickjs_wasm_name)));
  add_package_section(venom::package::SectionType::QuickJsWasmRuntime, "quickjs-wasm-runtime.vqwr", bytes_from_string(make_quickjs_wasm_runtime_metadata(profile, options.runtime, quickjs_wasm_name)));
  add_package_section(venom::package::SectionType::QuickJsEngineBackend, "quickjs-wasm-execution.vqwe", bytes_from_string(make_quickjs_wasm_execution_metadata(profile, options, release_policy, js_bridge, quickjs_wasm_name)));
  add_package_section(venom::package::SectionType::QuickJsSourceTransfer, "quickjs-source-transfer.vqst", bytes_from_string(make_quickjs_source_transfer_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsConsoleBridge, "quickjs-console-bridge.vqcb", bytes_from_string(make_quickjs_console_bridge_metadata(profile, options.runtime)));
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsExecutionRecords, "quickjs-execution-records.vqer", bytes_from_string(make_quickjs_execution_records_metadata(profile, options.runtime, js_bridge)));
  }
  add_package_section(venom::package::SectionType::QuickJsResultBridge, "quickjs-result-bridge.vqrb", bytes_from_string(make_quickjs_result_bridge_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsFallbackPolicy, "quickjs-fallback-policy.vqfp", bytes_from_string(make_quickjs_fallback_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsEngineBackend, "quickjs-engine-backend.vqeb", bytes_from_string(make_quickjs_engine_backend_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsNativeParity, "quickjs-native-parity.vqnp", bytes_from_string(make_quickjs_native_parity_metadata(profile, options.runtime, js_bridge, qjs_probe)));
  add_package_section(venom::package::SectionType::QuickJsExecutionMode, "quickjs-execution-mode.vqxm", bytes_from_string(make_quickjs_execution_mode_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsRuntimeAbi, "quickjs-runtime-abi.vqra", bytes_from_string(make_quickjs_runtime_abi_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsHostImports, "quickjs-host-imports.vqhi", bytes_from_string(make_quickjs_host_imports_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsHeapLimits, "quickjs-heap-limits.vqhl", bytes_from_string(make_quickjs_heap_limits_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsScriptBuffer, "quickjs-script-buffer.vqsb", bytes_from_string(make_quickjs_script_buffer_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsConsoleAbi, "quickjs-console-abi.vqca", bytes_from_string(make_quickjs_console_abi_metadata(profile, options.runtime)));
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsParityProbe, "quickjs-parity-probe.vqpp", bytes_from_string(make_quickjs_parity_probe_metadata(profile, options.runtime, qjs_probe)));
  }
  }
  add_package_section(venom::package::SectionType::QuickJsReleaseFallback, "quickjs-release-fallback.vqrf", bytes_from_string(make_quickjs_release_fallback_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsBytecodeManifest, "quickjs-bytecode-manifest.vqbm", bytes_from_string(make_quickjs_bytecode_manifest_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsModuleResolver, "quickjs-module-resolver.vqmr", bytes_from_string(make_quickjs_module_resolver_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsExceptionAbi, "quickjs-exception-abi.vqex", bytes_from_string(make_quickjs_exception_abi_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsHostTrapPolicy, "quickjs-host-trap-policy.vqtp", bytes_from_string(make_quickjs_host_trap_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsExecutionLifecycle, "quickjs-execution-lifecycle.vqel", bytes_from_string(make_quickjs_execution_lifecycle_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsScriptBufferPolicy, "quickjs-script-buffer-policy.vqsp", bytes_from_string(make_quickjs_script_buffer_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsContextLimitPolicy, "quickjs-context-limit-policy.vqlp", bytes_from_string(make_quickjs_context_limit_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsHostCallDispatch, "quickjs-host-call-dispatch.vqhd", bytes_from_string(make_quickjs_host_call_dispatch_metadata(profile, options.runtime)));
  if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsParityContract, "quickjs-parity-contract.vqpc", bytes_from_string(make_quickjs_parity_contract_metadata(profile, options.runtime)));
  }
  add_package_section(venom::package::SectionType::QuickJsReleaseFailClosed, "quickjs-release-failclosed.vqfc", bytes_from_string(make_quickjs_release_failclosed_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsModuleGraph, "quickjs-module-graph.vqmg", bytes_from_string(make_quickjs_module_graph_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsModuleExecution, "quickjs-module-execution.vqmx", bytes_from_string(make_quickjs_module_execution_metadata(profile, options.runtime, js_bridge)));
  if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsModuleCache, "quickjs-module-cache.vqmc", bytes_from_string(make_quickjs_module_cache_metadata(profile, options.runtime, js_bridge)));
  }
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsResolverAudit, "quickjs-resolver-audit.vqra", bytes_from_string(make_quickjs_resolver_audit_metadata(profile, options.runtime, js_bridge)));
  }
  }
  add_package_section(venom::package::SectionType::QuickJsInteropFallback, "quickjs-interop-fallback.vqif", bytes_from_string(make_quickjs_interop_fallback_metadata(profile, options.runtime)));
  if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsExecutionPipeline, "quickjs-execution-pipeline.vqxp", bytes_from_string(make_quickjs_execution_pipeline_metadata(profile, options.runtime, js_bridge)));
  }
  add_package_section(venom::package::SectionType::QuickJsScriptRecords, "quickjs-script-records.vqsr", bytes_from_string(make_quickjs_script_records_metadata(profile, options.runtime, js_bridge)));
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsEvalResults, "quickjs-eval-results.vqev", bytes_from_string(make_quickjs_eval_results_metadata(profile, options.runtime)));
  }
  }
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsConsoleCapture, "quickjs-console-capture.vqcc", bytes_from_string(make_quickjs_console_capture_metadata(profile, options.runtime)));
  }
  }
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsFailureReports, "quickjs-failure-reports.vqfr", bytes_from_string(make_quickjs_failure_reports_metadata(profile, options.runtime)));
  }
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsExecutionJournal, "quickjs-execution-journal.vqxj", bytes_from_string(make_quickjs_execution_journal_metadata(profile, options.runtime, js_bridge)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsCheckpointPolicy, "quickjs-checkpoint-policy.vqcp", bytes_from_string(make_quickjs_checkpoint_policy_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsReplayCursor, "quickjs-replay-cursor.vqrc", bytes_from_string(make_quickjs_replay_cursor_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsResumeState, "quickjs-resume-state.vqrs", bytes_from_string(make_quickjs_resume_state_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsDeterminismAudit, "quickjs-determinism-audit.vqda", bytes_from_string(make_quickjs_determinism_audit_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsSnapshotPolicy, "quickjs-snapshot-policy.vqsk", bytes_from_string(make_quickjs_snapshot_policy_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsSnapshotRecords, "quickjs-snapshot-records.vqsn", bytes_from_string(make_quickjs_snapshot_records_metadata(profile, options.runtime, js_bridge)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsReplayValidation, "quickjs-replay-validation.vqrv", bytes_from_string(make_quickjs_replay_validation_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsDeterminismLedger, "quickjs-determinism-ledger.vqdl", bytes_from_string(make_quickjs_determinism_ledger_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsAuditSeal, "quickjs-audit-seal.vqas", bytes_from_string(make_quickjs_audit_seal_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsExecutionCommit, "quickjs-execution-commit.vqxc", bytes_from_string(make_quickjs_execution_commit_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsRollbackPolicy, "quickjs-rollback-policy.vqrp", bytes_from_string(make_quickjs_rollback_policy_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsHostCallReceipts, "quickjs-host-call-receipts.vqhr", bytes_from_string(make_quickjs_host_call_receipts_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsReleaseAcceptance, "quickjs-release-acceptance.vqac", bytes_from_string(make_quickjs_release_acceptance_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsCommitAudit, "quickjs-commit-audit.vqca", bytes_from_string(make_quickjs_commit_audit_metadata(profile, options.runtime)));
  }
  add_package_section(venom::package::SectionType::QuickJsCapabilityPolicy, "quickjs-capability-policy.vqcpol", bytes_from_string(make_quickjs_capability_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsHostIoPolicy, "quickjs-host-io-policy.vqio", bytes_from_string(make_quickjs_host_io_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsPermissionSeal, "quickjs-permission-seal.vqps", bytes_from_string(make_quickjs_permission_seal_metadata(profile, options.runtime)));
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsPolicyReceipts, "quickjs-policy-receipts.vqpr", bytes_from_string(make_quickjs_policy_receipts_metadata(profile, options.runtime)));
  }
  add_package_section(venom::package::SectionType::QuickJsReleaseGate, "quickjs-release-gate.vqrg", bytes_from_string(make_quickjs_release_gate_metadata(profile, options.runtime)));
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsHostIoDecision, "quickjs-host-io-decision.vqid", bytes_from_string(make_quickjs_host_io_decision_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsHostIoDenyTrace, "quickjs-host-io-deny-trace.vqdt", bytes_from_string(make_quickjs_host_io_deny_trace_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsCapabilityLedger, "quickjs-capability-ledger.vqclg", bytes_from_string(make_quickjs_capability_ledger_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsPolicySealAudit, "quickjs-policy-seal-audit.vqpsa", bytes_from_string(make_quickjs_policy_seal_audit_metadata(profile, options.runtime)));
  }
  add_package_section(venom::package::SectionType::QuickJsRuntimeDenylist, "quickjs-runtime-denylist.vqrd", bytes_from_string(make_quickjs_runtime_denylist_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsContextLifecycle, "quickjs-context-lifecycle.vqcl", bytes_from_string(make_quickjs_context_lifecycle_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::HostCapabilities, "host-capabilities.vhcap", bytes_from_string(make_host_capabilities_metadata(profile, options.runtime, js_bridge)));
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsAdapterDiagnostics, "quickjs-adapter-diagnostics.vqad", bytes_from_string(make_quickjs_adapter_diagnostics_metadata(profile, options.runtime, js_bridge, quickjs_engine_name)));
  }
  add_package_section(venom::package::SectionType::ScriptEnginePolicy, "script-engine-policy.vsep", bytes_from_string(make_script_engine_policy_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::AsyncHostQueue, "async-host-queue.vahq", bytes_from_string(make_async_host_queue_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::Routes, "routes.vbrt", compiled_routes.route_table);
  add_package_section(venom::package::SectionType::Css, style_name, bytes_from_string(css));
  if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::JavaScript, "scripts.vjsb", js_bridge.bundle);
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::JavaScript, "script-diagnostics.txt", bytes_from_string(js_bridge.diagnostics));
    add_package_section(venom::package::SectionType::JavaScript, "bundle-preview.js", bytes_from_string(js_preview));
  }
  add_package_section(venom::package::SectionType::Strings, "strings.vstr", compiled_routes.string_table);
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::DomTemplates, "dom-templates.txt", bytes_from_string(compiled_routes.preview));
  }
  add_package_section(venom::package::SectionType::VmBytecode, "route-bytecode.vmbc", compiled_routes.bytecode);

  std::unordered_map<std::string, std::vector<JsChunk>> lazy_scripts_by_route;
  for (const auto& chunk : js_bridge.chunks) {
    lazy_scripts_by_route[chunk.route].push_back(chunk);
  }
  std::vector<LazySectionPlanRow> lazy_rows;
  lazy_rows.reserve(compiled_routes.routes.size());
  for (const auto& route : compiled_routes.routes) {
    LazySectionPlanRow row;
    row.route = route.route;
    row.instruction_count = route.instruction_count;
    if (compiled_routes.routes.size() == 1u) {
      row.route_section = "route-bytecode.vmbc";
      row.route_bytecode_size = static_cast<std::uint32_t>(compiled_routes.bytecode.size());
    } else {
      row.route_section = lazy_route_section_name(route.route);
      auto route_bytes = make_single_route_bytecode_section(route.program, poly);
      row.route_bytecode_size = static_cast<std::uint32_t>(route_bytes.size());
      add_package_section(venom::package::SectionType::VmBytecode, row.route_section, std::move(route_bytes));
    }

    const auto scripts_it = lazy_scripts_by_route.find(route.route);
    if (scripts_it != lazy_scripts_by_route.end() && !scripts_it->second.empty()) {
      row.script_section = lazy_script_section_name(route.route);
      row.script_count = static_cast<std::uint32_t>(scripts_it->second.size());
      add_package_section(venom::package::SectionType::JavaScript, row.script_section, encode_route_script_bundle(scripts_it->second, profile.kind == ProfileKind::Release || profile.strip_debug_metadata));
    }
    lazy_rows.push_back(std::move(row));
  }

  add_package_section(venom::package::SectionType::Integrity, "vm-polymorph.vpol", poly.encode_binary());
  if (profile.kind == ProfileKind::Release || profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::Integrity,
                        "release-diversification.vrd3",
                        make_release_diversification_table(poly));
    add_package_section(venom::package::SectionType::Integrity,
                        "quickjs-abi-fingerprint.vqaf",
                        make_quickjs_abi_fingerprint());
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJs, "quickjs-probe.txt", bytes_from_string(qjs_probe));
    add_package_section(venom::package::SectionType::QuickJs, "quickjs-bridge-plan.txt", bytes_from_string("QuickJS native embedding is available for compile-time probes. Browser QuickJS execution now loads the v0.46 bytecode-record pipeline with the release-policy-gated QuickJS/WASM host-I/O decision enforcement boundary with ABI-table-driven route-scoped script context records, heap/accounting limits, script-buffer allocation, console callback ABI, and parity probes, bytecode manifests, module resolver contracts, exception records, and host-trap policy; the AEAD provider status remains honest.\n"));
  }
  add_package_section(venom::package::SectionType::AssetManifest, "asset-manifest.txt", bytes_from_string(assets.manifest_text));

  for (const auto& file : graph.files) {
    if (!is_code_or_markup(file)) {
      add_package_section(venom::package::SectionType::Asset, file.relative, file.bytes);
    }
  }

  constexpr std::uint32_t kPolymorphicLayoutMaxPadding = 31u;
  add_package_section(venom::package::SectionType::Integrity,
                      "package-layout.vlay",
                      bytes_from_string(make_package_layout_metadata(profile, options, poly, package_sections, kPolymorphicLayoutMaxPadding)));

  add_package_section(venom::package::SectionType::Integrity,
                      "lazy-sections.vlazy",
                      bytes_from_string(make_lazy_sections_metadata(profile, options, lazy_rows)));

  add_package_section(venom::package::SectionType::PackageFeatures,
                      "package-features.vfeat",
                      bytes_from_string(make_package_features_metadata(profile, options.runtime, package_sections, options.crypto_provider)));

  if (profile.integrity_metadata) {
    add_package_section(venom::package::SectionType::Integrity,
                        "integrity-auth.vsig",
                        bytes_from_string(make_integrity_metadata(profile, options.runtime, package_sections, options.crypto_provider)));
  }

  if (profile.shuffle_sections) {
    shuffle_package_sections(package_sections, poly);
  }

  venom::package::Writer writer;
  writer.set_flags(package_flags);
  writer.set_compression_enabled(profile.compress_sections);
  writer.set_section_encryption_enabled(profile.crypto_provider_ready && profile.kind != ProfileKind::Debug);
  writer.set_section_name_redaction_enabled(profile.strip_debug_metadata && profile.kind != ProfileKind::Debug);
  writer.set_layout_polymorphism_enabled(profile.polymorphic && profile.kind != ProfileKind::Debug);
  writer.set_layout_seed(poly.seed);
  writer.set_layout_max_padding(kPolymorphicLayoutMaxPadding);
  writer.set_section_crypto_provider(selected_writer_crypto_provider(options.crypto_provider));
  for (auto& section : package_sections) {
    writer.add_section(section.type, section.flags, std::move(section.name), std::move(section.data));
  }

  if (hardened_release_asset) {
    std::filesystem::create_directories(assets_dir / "app");
    std::filesystem::create_directories(assets_dir / "style");
    std::filesystem::create_directories(assets_dir / "loader");
    std::filesystem::create_directories(assets_dir / "runtime");
    std::filesystem::create_directories(assets_dir / "workers");
  }
  const auto package_temp_path = hardened_release_asset ? (assets_dir / "app" / ".app.vbc.tmp") : (assets_dir / ".app.vbc.tmp");
  writer.write(package_temp_path);
  const auto package_bytes = read_bytes(package_temp_path);
  const auto package_probe = venom::package::read_package(package_temp_path);
  const auto package_file_name = named_output("app", ".vbc", package_bytes, options.hashed_assets);
  const auto package_name = hardened_release_asset ? "app/" + package_file_name : package_file_name;
  const auto package_path = assets_dir / package_name;
  std::filesystem::rename(package_temp_path, package_path);

  const auto loader_runtime_ref = hardened_release_asset ? "../" + runtime_name : runtime_name;
  const auto loader_package_ref = hardened_release_asset ? "../" + package_name : package_name;
  const auto loader_engine_ref = hardened_release_asset ? "../" + quickjs_engine_name : quickjs_engine_name;
  const auto loader_qjs_wasm_ref = hardened_release_asset ? "../" + quickjs_wasm_name : quickjs_wasm_name;
  const auto loader_runtime_wasm_ref = hardened_release_asset ? "../" + wasm_name : wasm_name;
  const auto loader_style_ref = hardened_release_asset ? "../" + style_name : style_name;
  const auto loader_worker_ref = hardened_release_asset ? "../" + worker_name : std::string{};
  auto loader = make_loader_js(loader_runtime_ref, loader_package_ref, package_probe.package_hash, venom::package::sha256_hex(package_bytes), profile.runtime_hardened || profile.fail_closed, loader_engine_ref, loader_qjs_wasm_ref, loader_runtime_wasm_ref, loader_style_ref, package_binding_token, loader_worker_ref, venom::package::sha256_hex(quickjs_wasm_bytes), js_bridge.protected_exports,
      bridge_invoke_opcode, bridge_cancel_opcode, bridge_result_opcode, bridge_error_opcode);
  if (hardened_release_asset) {
    loader = harden_release_js_asset(std::move(loader));
    loader = ast_harden_release_js("loader", loader);
    validate_protected_js_asset("loader", loader);
  }
  const auto loader_file_name = named_output("loader", ".js", loader, options.hashed_assets);
  const auto loader_name = hardened_release_asset ? "loader/" + loader_file_name : loader_file_name;
  const auto html = make_bootstrap_html(graph,
                                        "assets/" + loader_name,
                                        "assets/" + style_name,
                                        venom::package::sha256_sri(bytes_from_string(loader)),
                                        venom::package::sha256_sri(bytes_from_string(css)));

  write_text(options.output / "index.html", html);
  if (!hardened_release_asset) {
    emit_html_route_shells(graph, options.output, loader_name, style_name);
    write_text(options.output / "sw.js", make_service_worker_js());
    write_text(options.output / "favicon.ico", "");
  }
  write_text(assets_dir / style_name, css);
  write_text(assets_dir / loader_name, loader);
  write_text(assets_dir / runtime_name, runtime);
  write_text(assets_dir / quickjs_engine_name, quickjs_engine_module);
  if (hardened_release_asset) write_text(assets_dir / worker_name, worker_runtime);
  {
    std::ofstream qjs_wasm_out(assets_dir / quickjs_wasm_name, std::ios::binary);
    if (!qjs_wasm_out) {
      throw std::runtime_error("failed to write " + (assets_dir / quickjs_wasm_name).string());
    }
    qjs_wasm_out.write(reinterpret_cast<const char*>(quickjs_wasm_bytes.data()), static_cast<std::streamsize>(quickjs_wasm_bytes.size()));
    qjs_wasm_out.flush();
    if (!qjs_wasm_out) {
      throw std::runtime_error("failed to finish writing " + (assets_dir / quickjs_wasm_name).string());
    }
  }
  {
    const auto written_quickjs_wasm = read_bytes(assets_dir / quickjs_wasm_name);
    if (written_quickjs_wasm != quickjs_wasm_bytes) {
      throw std::runtime_error("QuickJS WASM write verification failed");
    }
    const auto written_digest = venom::package::sha256_hex(written_quickjs_wasm);
    const auto expected_digest = venom::package::sha256_hex(quickjs_wasm_bytes);
    if (written_digest != expected_digest) {
      throw std::runtime_error("QuickJS WASM digest changed after package binding generation");
    }
  }
  if (wasm_runtime) {
    std::ofstream wasm_out(assets_dir / wasm_name, std::ios::binary);
    if (!wasm_out) {
      throw std::runtime_error("failed to write " + (assets_dir / wasm_name).string());
    }
    wasm_out.write(reinterpret_cast<const char*>(wasm_bytes.data()), static_cast<std::streamsize>(wasm_bytes.size()));
  }
  if (should_emit_external_asset_manifest(profile, options)) {
    write_text(assets_dir / "asset-manifest.txt", assets.manifest_text);
  }
  emit_public_assets(assets, assets_dir);

  const auto validated_package = venom::package::read_package(package_path);
  if (!vendor_lock.present || options.refresh_vendors) {
    try {
      write_remote_vendor_lock(vendor_lock_path, js_bridge.remote_vendors);
    } catch (...) {
      std::error_code cleanup_error;
      std::filesystem::remove_all(options.output, cleanup_error);
      throw;
    }
  }
  std::ostringstream provenance;
  provenance << "{\n"
             << "  \"schema_version\": 1,\n"
             << "  \"product\": \"" << json_escape(VENOM_PRODUCT_NAME) << "\",\n"
             << "  \"venom_version\": \"" << VENOM_VERSION_STRING << "\",\n"
             << "  \"package_format_version\": " << validated_package.version << ",\n"
             << "  \"runtime_abi_version\": " << validated_package.runtime_abi << ",\n"
             << "  \"profile\": \"" << json_escape(profile.name) << "\",\n"
             << "  \"security_target\": \"" << json_escape(options.security_target) << "\",\n"
             << "  \"quickjs_backend\": \"" << json_escape(release_policy.backend) << "\",\n"
             << "  \"runtime_modules\": " << runtime_module_plan_json(runtime_modules) << ",\n"
             << "  \"module_graph\": {\"chunks\": " << js_bridge.chunks.size()
             << ", \"edges\": " << js_bridge.module_edges.size()
             << ", \"dynamic_literal_edges\": " << std::count_if(js_bridge.module_edges.begin(), js_bridge.module_edges.end(), [](const auto& edge) { return edge.dynamic; }) << "},\n"
             << "  \"vendor_lock_sha256\": \"" << vendor_lock_digest << "\",\n"
             << "  \"package_sha256\": \"" << venom::package::sha256_hex(package_bytes) << "\",\n"
             << "  \"package_asset\": \"assets/" << json_escape(package_name) << "\",\n"
             << "  \"reproducible_timestamp_source\": \"SOURCE_DATE_EPOCH\"\n"
             << "}\n";
  const auto build_metadata_ref = hardened_release_asset ? std::string{"assets/app/build.json"} : std::string{"assets/build.json"};
  const auto build_metadata_path = options.output / build_metadata_ref;
  write_text(build_metadata_path, provenance.str());
  if (!hardened_release_asset) {
    const auto report_dir = options.output / "build" / "reports";
    write_text(report_dir / "execution-plan.txt", js_bridge.execution_plan_text);
    write_text(report_dir / "execution-plan.json", js_bridge.execution_plan_json);
    write_text(report_dir / "function-plan.txt", js_bridge.function_plan_text);
    write_text(report_dir / "function-plan.json", js_bridge.function_plan_json);
    write_text(report_dir / "function-extraction-plan.txt", js_bridge.extraction_plan_text);
    write_text(report_dir / "function-extraction-plan.json", js_bridge.extraction_plan_json);
    write_text(report_dir / "realm-bridge-contract.json", js_bridge.realm_bridge_contract_json);
    write_text(report_dir / "bridge-rewrite-plan.json", js_bridge.bridge_rewrite_report_json);
  }

  if (options.format != OutputFormat::Json) {
    print_build_protection_report(profile, options, package_path, package_bytes, validated_package, should_emit_external_asset_manifest(profile, options));
  }

  if (options.format == OutputFormat::Json) {
    std::cout << "{\"ok\":true,\"version\":\"" << VENOM_VERSION_STRING
              << "\",\"output\":\"" << json_escape(options.output.generic_string())
              << "\",\"package\":\"assets/" << json_escape(package_name)
              << "\",\"package_sha256\":\"" << venom::package::sha256_hex(package_bytes)
              << "\",\"build_metadata\":\"" << json_escape(build_metadata_ref) << "\"}\n";
    return true;
  }

  std::cout << "venom: vendor lock => " << vendor_lock_path.string()
            << " mode=" << vendor_lock_mode
            << " sha256=" << vendor_lock_digest << "\n"
            << "venom: built " << graph.files.size() << " files, "
            << graph.routes.size() << " routes, " << assets.records.size() << " assets, "
            << js_bridge.chunks.size() << " script chunks ("
            << compiled_routes.bytecode.size() << " route bytecode bytes) using profile '" << profile.name << "'\n"
            << "venom: package v" << validated_package.version << " abi " << validated_package.runtime_abi
            << " sections " << validated_package.sections.size()
            << " bytes " << validated_package.file_size << "\n"
            << "venom: dist assets => " << loader_name << ", " << runtime_name << ", " << package_name << ", " << style_name << ", " << quickjs_engine_name << ", " << quickjs_wasm_name << (hardened_release_asset ? ", " + worker_name : "") << (hardened_release_asset ? "" : ", sw.js");
  if (wasm_runtime) {
    std::cout << ", " << wasm_name;
  }
  std::cout << "\n"
            << "venom: profile => compressed=" << (profile.compress_sections ? "yes" : "no")
            << " fail_closed=" << (profile.fail_closed ? "yes" : "no")
            << " integrity_provider_ready=" << (profile.integrity_metadata ? "yes" : "no")
            << " integrity_metadata=" << (profile.integrity_metadata ? "yes" : "no")
            << " runtime_hardened=" << (profile.runtime_hardened ? "yes" : "no")
            << " strip_debug_metadata=" << (profile.strip_debug_metadata ? "yes" : "no")
            << " sealed_sections=" << ((profile.crypto_provider_ready && profile.kind != ProfileKind::Debug) ? "yes" : "no")
            << " crypto_provider=" << selected_section_sealer_name(profile, options.crypto_provider)
            << " external_debug_manifest=" << (should_emit_external_asset_manifest(profile, options) ? "yes" : "no")
            << " aead_provider_ready=" << (profile.aead_provider_ready ? "yes" : "no")
            << " signature_provider_ready=no"
            << " runtime=" << options.runtime
            << " quickjs_backend=" << release_policy.backend
            << " host_fallback=" << (release_policy.fallback_allowed ? "allowed" : "denied")
            << " release_policy=" << release_policy.decision << "\n"
            << "venom: quickjs probe => " << qjs_probe << "\n"
            << "venom: output => " << options.output.string() << "\n";
  return true;
}

} // namespace venom::compiler

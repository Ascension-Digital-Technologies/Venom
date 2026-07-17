#pragma once

#include "compiler/pipeline/build.hpp"
#include "compiler/pipeline/js.hpp"
#include "compiler/core/profile.hpp"
#include "compiler/core/site.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace venom::compiler::build_detail {

std::string json_escape(const std::string& value);
void write_text(const std::filesystem::path& path, const std::string& content);
std::vector<unsigned char> read_bytes(const std::filesystem::path& path);
std::vector<unsigned char> bytes_from_string(const std::string& value);
std::string metadata_value_from_text(const std::string& text, const std::string& key);
std::string wasm_truth_value(const std::string& key, const std::string& fallback);
bool wasm_truth_bool(const std::string& key, bool fallback);
void require_real_embedded_quickjs_runtime();
std::uint32_t bridge_protocol_opcode(const std::string& salt, const std::string& label);
void require_embedded_quickjs_module_bundle_runtime(const JsBridge& bridge);
std::string named_output(const std::string& stem, const std::string& ext, const std::vector<unsigned char>& bytes, bool hashed);
std::string named_output(const std::string& stem, const std::string& ext, const std::string& text, bool hashed);
void replace_all_inplace(std::string& value, const std::string& from, const std::string& to);
std::uint32_t read_wasm_uleb(const std::vector<unsigned char>& bytes, std::size_t& cursor, std::size_t limit);
std::string compact_wasm_export_alias(const std::string& name, std::size_t ordinal, const std::string& salt);
std::vector<std::pair<std::string, std::string>> compact_quickjs_wasm_exports(std::vector<unsigned char>& bytes, const std::string& salt);
std::vector<std::pair<std::string, std::string>> rewrite_wasm_export_names(std::vector<unsigned char>& bytes, const std::string& salt);
void apply_wasm_export_aliases(std::string& js, const std::vector<std::pair<std::string, std::string>>& aliases);
void redact_unmapped_quickjs_symbols(std::string& js, const std::string& salt);
void redact_quickjs_wasm_abi_table(std::vector<unsigned char>& bytes, const std::string& salt);
bool js_identifier_char(char ch);
bool js_regex_prefix(char ch);
std::string minify_release_js(const std::string& input);
std::string harden_release_js_asset(std::string js);
std::filesystem::path find_js_hardener_script();
std::string ast_harden_release_js(const std::string& kind, const std::string& js);
bool redact_release_metadata(const Profile& profile);
std::string protected_route_label(const Profile& profile, const JsChunk& chunk);
std::string protected_source_label(const Profile& profile, const JsChunk& chunk);
std::size_t protected_source_size(const Profile& profile, const JsChunk& chunk);
std::uint64_t protected_source_hash(const Profile& profile, const JsChunk& chunk);
void validate_protected_js_asset(const std::string& name, const std::string& js);
bool is_code_or_markup(const SiteFile& file);
bool is_html_route_file(const SiteFile& file);
std::string route_shell_asset_prefix(const std::filesystem::path& relative_html_path);
void emit_html_route_shells(const SiteGraph& graph,
                            const std::filesystem::path& output_dir,
                            const std::string& loader_name,
                            const std::string& style_name);
std::string make_service_worker_js();

} // namespace venom::compiler::build_detail

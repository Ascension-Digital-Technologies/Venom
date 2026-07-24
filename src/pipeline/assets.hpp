#pragma once

#include "core/site.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace venom::compiler {

struct JsChunk;

struct AssetRecord {
  std::string source;
  std::string output_name;
  std::string public_path;
  std::string css_url;
  std::string mime;
  std::uint64_t hash = 0;
  std::uint64_t size = 0;
  const SiteFile* file = nullptr;
};

struct AssetPipeline {
  bool hashed = false;
  std::vector<AssetRecord> records;
  std::unordered_map<std::string, std::size_t> by_source;
  std::string manifest_text;
};

AssetPipeline build_asset_pipeline(const SiteGraph& graph,
                                   const std::vector<JsChunk>& planned_scripts,
                                   bool hashed,
                                   bool production,
                                   bool standard_layout_only = false);
const AssetRecord* find_asset(const AssetPipeline& pipeline, const std::string& source);
std::string detect_mime_type(const std::string& relative_path);
std::string short_hash_hex(std::uint64_t value, std::size_t digits = 12);
void emit_public_assets(const AssetPipeline& pipeline, const std::filesystem::path& output_assets_dir);

} // namespace venom::compiler

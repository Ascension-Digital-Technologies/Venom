#include "venom/base/error.hpp"
#include "venom/internal/pipeline/assets.hpp"

#include "venom/package/hash.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace venom::compiler {

namespace {
std::string normalize_slashes(std::string value) {
  std::replace(value.begin(), value.end(), '\\', '/');
  while (!value.empty() && value.front() == '/') {
    value.erase(value.begin());
  }
  while (value.rfind("./", 0) == 0) {
    value.erase(0, 2);
  }
  return value;
}

std::string lower_ext(std::filesystem::path path) {
  auto ext = path.extension().generic_string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return ext;
}

bool is_deployment_metadata(const SiteFile& file) {
  const auto relative = normalize_slashes(file.relative);
  return relative == "venom.browser.json";
}

bool is_third_party_notice(const SiteFile& file) {
  auto relative = normalize_slashes(file.relative);
  std::transform(relative.begin(), relative.end(), relative.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  const auto has_suffix = [&](const std::string& suffix) {
    return relative.size() >= suffix.size() &&
           relative.compare(relative.size() - suffix.size(), suffix.size(), suffix) == 0;
  };
  return relative == "third_party_notices.txt" ||
         relative == "third-party-notices.txt" ||
         has_suffix("/third_party_notices.txt") ||
         has_suffix("/third-party-notices.txt");
}

bool is_public_asset(const SiteFile& file, bool production, bool standard_layout_only) {
  // Chrome-extension distributions intentionally contain only Venom's generated
  // production assets. Extension-owned loose files are emitted separately by the
  // extension target using a strict runtime allowlist, so repository metadata and
  // arbitrary project files never become hashed assets.
  if (standard_layout_only) return false;
  if (production && (is_deployment_metadata(file) || is_third_party_notice(file))) return false;
  return
         file.extension != ".html" && file.extension != ".htm" &&
         file.extension != ".css" &&
         file.extension != ".js" && file.extension != ".mjs" && file.extension != ".cjs" &&
         file.extension != ".jsx" && file.extension != ".ts" && file.extension != ".tsx" &&
         file.extension != ".mts" && file.extension != ".cts";
}

bool is_image_asset(const std::string& mime) {
  return mime.rfind("image/", 0) == 0;
}

std::string safe_stem(std::string relative) {
  relative = normalize_slashes(std::move(relative));
  const auto ext_pos = relative.find_last_of('.');
  if (ext_pos != std::string::npos) {
    relative.resize(ext_pos);
  }
  for (char& c : relative) {
    const auto u = static_cast<unsigned char>(c);
    if (std::isalnum(u) == 0 && c != '-' && c != '_') {
      c = '_';
    }
  }
  while (!relative.empty() && relative.front() == '_') relative.erase(relative.begin());
  while (!relative.empty() && relative.back() == '_') relative.pop_back();
  if (relative.empty()) relative = "asset";
  return relative;
}

void write_bytes(const std::filesystem::path& path, const std::vector<unsigned char>& bytes) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    raise_error("VENOM-E5000", "failed to write asset " + path.string());
  }
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

std::string make_manifest(const std::vector<AssetRecord>& records) {
  std::ostringstream out;
  out << "VASM0005\n";
  out << "version\t1\n";
  out << "count\t" << records.size() << "\n";
  out << "source\toutput\tpublic\tmime\tsize\thash\n";
  for (const auto& record : records) {
    out << record.source << '\t'
        << record.output_name << '\t'
        << record.public_path << '\t'
        << record.mime << '\t'
        << record.size << '\t'
        << short_hash_hex(record.hash, 16)
        << "\n";
  }
  return out.str();
}
} // namespace

std::string short_hash_hex(std::uint64_t value, std::size_t digits) {
  std::ostringstream out;
  out << std::hex << std::setw(16) << std::setfill('0') << value;
  auto text = out.str();
  if (digits > 0 && digits < text.size()) {
    text.resize(digits);
  }
  return text;
}

std::string detect_mime_type(const std::string& relative_path) {
  const auto ext = lower_ext(std::filesystem::path(relative_path));
  if (ext == ".svg") return "image/svg+xml";
  if (ext == ".png") return "image/png";
  if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
  if (ext == ".gif") return "image/gif";
  if (ext == ".webp") return "image/webp";
  if (ext == ".avif") return "image/avif";
  if (ext == ".ico") return "image/x-icon";
  if (ext == ".woff") return "font/woff";
  if (ext == ".woff2") return "font/woff2";
  if (ext == ".ttf") return "font/ttf";
  if (ext == ".otf") return "font/otf";
  if (ext == ".eot") return "application/vnd.ms-fontobject";
  if (ext == ".json") return "application/json";
  if (ext == ".xml") return "application/xml";
  if (ext == ".pdf") return "application/pdf";
  if (ext == ".txt") return "text/plain; charset=utf-8";
  if (ext == ".wasm") return "application/wasm";
  if (ext == ".mp4") return "video/mp4";
  if (ext == ".webm") return "video/webm";
  if (ext == ".mp3") return "audio/mpeg";
  if (ext == ".wav") return "audio/wav";
  return "application/octet-stream";
}

AssetPipeline build_asset_pipeline(const SiteGraph& graph, bool hashed, bool production, bool standard_layout_only) {
  AssetPipeline pipeline;
  pipeline.hashed = hashed;
  std::unordered_set<std::string> used_names;

  for (const auto& file : graph.files) {
    if (!is_public_asset(file, production, standard_layout_only)) {
      continue;
    }

    AssetRecord record;
    record.source = normalize_slashes(file.relative);
    record.hash = venom::package::fnv1a64(file.bytes);
    record.size = static_cast<std::uint64_t>(file.bytes.size());
    record.mime = detect_mime_type(file.relative);
    record.file = &file;

    const auto ext = std::filesystem::path(file.relative).extension().generic_string();
    const auto base = safe_stem(file.relative);
    if (hashed) {
      record.output_name = base + "." + short_hash_hex(record.hash, 12) + ext;
    } else {
      record.output_name = normalize_slashes(file.relative);
      for (char& c : record.output_name) {
        if (c == '/') c = '_';
      }
    }

    while (used_names.find(record.output_name) != used_names.end()) {
      record.output_name = safe_stem(file.relative) + "." + short_hash_hex(record.hash, 12) + "." + std::to_string(used_names.size()) + ext;
    }
    used_names.insert(record.output_name);

    if (is_image_asset(record.mime)) {
      record.output_name = "images/" + record.output_name;
      record.public_path = "assets/" + record.output_name;
      // Production CSS is emitted under assets/app/, so image URLs must step
      // back to the shared assets root before entering assets/images/.
      record.css_url = "../" + record.output_name;
    } else if (is_third_party_notice(file)) {
      record.output_name = "app/third-party-notices.txt";
      record.public_path = "assets/" + record.output_name;
      record.css_url = "../" + record.output_name;
    } else {
      record.public_path = "assets/" + record.output_name;
      record.css_url = "../" + record.output_name;
    }
    pipeline.by_source[record.source] = pipeline.records.size();
    pipeline.records.push_back(std::move(record));
  }

  std::sort(pipeline.records.begin(), pipeline.records.end(), [](const AssetRecord& a, const AssetRecord& b) {
    return a.source < b.source;
  });
  pipeline.by_source.clear();
  for (std::size_t i = 0; i < pipeline.records.size(); ++i) {
    pipeline.by_source[pipeline.records[i].source] = i;
  }
  pipeline.manifest_text = make_manifest(pipeline.records);
  return pipeline;
}

const AssetRecord* find_asset(const AssetPipeline& pipeline, const std::string& source) {
  const auto normalized = normalize_slashes(source);
  const auto it = pipeline.by_source.find(normalized);
  if (it == pipeline.by_source.end()) {
    return nullptr;
  }
  return &pipeline.records[it->second];
}

void emit_public_assets(const AssetPipeline& pipeline, const std::filesystem::path& output_assets_dir) {
  std::filesystem::create_directories(output_assets_dir);
  for (const auto& record : pipeline.records) {
    if (record.file == nullptr) {
      raise_error("VENOM-E5000", "asset pipeline record is missing source file: " + record.source);
    }
    write_bytes(output_assets_dir / record.output_name, record.file->bytes);
  }
}

} // namespace venom::compiler

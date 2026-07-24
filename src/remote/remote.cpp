#include "base/error.hpp"
#include "remote/remote.hpp"
#include "core/version.hpp"

#include "core/process.hpp"
#include "package/hash.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace venom::compiler {
namespace {

std::string lower_ascii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

std::vector<unsigned char> bytes_from_string(const std::string& value) {
  return std::vector<unsigned char>(value.begin(), value.end());
}

std::vector<unsigned char> read_bytes(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    raise_error("VENOM-E7000", "failed to read remote vendor file: " + path.string());
  }
  return std::vector<unsigned char>(std::istreambuf_iterator<char>(input), {});
}

std::string read_text(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    return {};
  }
  return std::string(std::istreambuf_iterator<char>(input), {});
}

void write_bytes_atomic(const std::filesystem::path& path, const std::vector<unsigned char>& bytes) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  static std::atomic<std::uint64_t> sequence{0};
  std::random_device random;
  std::ostringstream temporary_name;
  temporary_name << path.string() << "." << std::hex << random() << sequence.fetch_add(1) << ".tmp";
  const std::filesystem::path temporary = temporary_name.str();
  {
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) {
      raise_error("VENOM-E7000", "failed to write remote vendor file: " + temporary.string());
    }
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!output) {
      raise_error("VENOM-E7000", "failed to write remote vendor bytes: " + temporary.string());
    }
  }
  std::error_code ec;
  std::filesystem::remove(path, ec);
  std::filesystem::rename(temporary, path);
}

void write_text_atomic(const std::filesystem::path& path, const std::string& text) {
  write_bytes_atomic(path, bytes_from_string(text));
}

std::string metadata_value(const std::string& text, const std::string& key) {
  std::stringstream input(text);
  std::string line;
  const std::string prefix = key + "=";
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.rfind(prefix, 0) == 0) {
      return line.substr(prefix.size());
    }
  }
  return {};
}

std::vector<std::string> split_tabs(const std::string& line) {
  std::vector<std::string> fields;
  std::size_t start = 0;
  while (true) {
    const auto tab = line.find('\t', start);
    if (tab == std::string::npos) {
      fields.push_back(line.substr(start));
      break;
    }
    fields.push_back(line.substr(start, tab - start));
    start = tab + 1;
  }
  return fields;
}

std::size_t parse_size_value(const std::string& value, const std::string& context) {
  if (value.empty() || !std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isdigit(c) != 0; })) {
    raise_error("VENOM-E7000", "invalid numeric value in " + context);
  }
  try {
    return static_cast<std::size_t>(std::stoull(value));
  } catch (const std::exception&) {
    raise_error("VENOM-E7000", "numeric value out of range in " + context);
  }
}

bool is_hex_digest(const std::string& value, std::size_t digits) {
  return value.size() == digits && std::all_of(value.begin(), value.end(), [](unsigned char c) {
    return std::isdigit(c) != 0 || (c >= 'a' && c <= 'f');
  });
}

bool valid_utf8(const std::vector<unsigned char>& bytes) {
  std::size_t i = 0;
  while (i < bytes.size()) {
    const unsigned char c = bytes[i++];
    if (c <= 0x7fu) continue;
    std::size_t remaining = 0;
    std::uint32_t codepoint = 0;
    if ((c & 0xe0u) == 0xc0u) {
      remaining = 1;
      codepoint = c & 0x1fu;
      if (codepoint == 0u) return false;
    } else if ((c & 0xf0u) == 0xe0u) {
      remaining = 2;
      codepoint = c & 0x0fu;
    } else if ((c & 0xf8u) == 0xf0u) {
      remaining = 3;
      codepoint = c & 0x07u;
    } else {
      return false;
    }
    if (i + remaining > bytes.size()) return false;
    for (std::size_t j = 0; j < remaining; ++j) {
      const unsigned char next = bytes[i++];
      if ((next & 0xc0u) != 0x80u) return false;
      codepoint = (codepoint << 6u) | (next & 0x3fu);
    }
    if ((remaining == 1 && codepoint < 0x80u) ||
        (remaining == 2 && codepoint < 0x800u) ||
        (remaining == 3 && codepoint < 0x10000u) ||
        codepoint > 0x10ffffu ||
        (codepoint >= 0xd800u && codepoint <= 0xdfffu)) {
      return false;
    }
  }
  return true;
}

void validate_script_bytes(const std::vector<unsigned char>& bytes,
                           std::size_t max_script_bytes,
                           const std::string& url) {
  if (bytes.empty()) {
    raise_error("VENOM-E7000", "remote script is empty: " + url);
  }
  if (bytes.size() > max_script_bytes) {
    raise_error("VENOM-E7000", "remote script exceeds the production vendor size limit: " + url);
  }
  if (std::find(bytes.begin(), bytes.end(), static_cast<unsigned char>(0)) != bytes.end()) {
    raise_error("VENOM-E7000", "remote script contains NUL bytes: " + url);
  }
  if (!valid_utf8(bytes)) {
    raise_error("VENOM-E7000", "remote script is not valid UTF-8: " + url);
  }

  const std::size_t prefix_size = std::min<std::size_t>(bytes.size(), 1024u);
  std::string prefix(reinterpret_cast<const char*>(bytes.data()), prefix_size);
  prefix = lower_ascii(std::move(prefix));
  const auto first = prefix.find_first_not_of(" \t\r\n");
  if (first != std::string::npos) {
    const auto trimmed = prefix.substr(first);
    if (trimmed.rfind("<!doctype html", 0) == 0 || trimmed.rfind("<html", 0) == 0 ||
        trimmed.rfind("<head", 0) == 0 || trimmed.rfind("<body", 0) == 0) {
      raise_error("VENOM-E7000", "remote script request returned HTML instead of JavaScript: " + url);
    }
  }
}

std::string url_host(const std::string& normalized_url) {
  constexpr std::string_view prefix = "https://";
  const auto authority_start = prefix.size();
  const auto authority_end = normalized_url.find_first_of("/?#", authority_start);
  const auto authority = normalized_url.substr(authority_start, authority_end - authority_start);
  if (authority.empty()) return {};
  if (authority.find('@') != std::string::npos) {
    raise_error("VENOM-E7000", "remote script URL credentials are denied");
  }
  if (authority.front() == '[') {
    raise_error("VENOM-E7000", "remote script IPv6 literals are denied");
  }
  const auto colon = authority.find(':');
  return lower_ascii(authority.substr(0, colon));
}

bool looks_like_private_ipv4(const std::string& host) {
  std::vector<unsigned int> octets;
  std::stringstream input(host);
  std::string part;
  while (std::getline(input, part, '.')) {
    if (part.empty() || part.size() > 3 || !std::all_of(part.begin(), part.end(), [](unsigned char c) { return std::isdigit(c) != 0; })) {
      return false;
    }
    const auto value = static_cast<unsigned int>(std::stoul(part));
    if (value > 255u) return false;
    octets.push_back(value);
  }
  if (octets.size() != 4u) return false;
  const auto a = octets[0];
  const auto b = octets[1];
  return a == 0u || a == 10u || a == 127u ||
         (a == 100u && b >= 64u && b <= 127u) ||
         (a == 169u && b == 254u) ||
         (a == 172u && b >= 16u && b <= 31u) ||
         (a == 192u && b == 168u) || a >= 224u;
}

void validate_public_https_url(const std::string& normalized_url) {
  if (normalized_url.size() > 4096u) {
    raise_error("VENOM-E7000", "remote script URL exceeds 4096 bytes");
  }
  for (const char raw_c : normalized_url) {
    const auto c = static_cast<unsigned char>(raw_c);
    if (c < 0x20u || c == 0x7fu || std::isspace(c) != 0 || c == '\\') {
      raise_error("VENOM-E7000", "remote script URL contains forbidden whitespace or control characters");
    }
  }
  if (normalized_url.rfind("https://", 0) != 0) {
    raise_error("VENOM-E7000", "production remote script vendoring requires HTTPS: " + normalized_url);
  }
  const auto host = url_host(normalized_url);
  if (host.empty()) {
    raise_error("VENOM-E7000", "remote script URL is missing a host: " + normalized_url);
  }
  if (host == "localhost" ||
      (host.size() >= 6u && host.substr(host.size() - 6u) == ".local") ||
      (host.size() >= 10u && host.substr(host.size() - 10u) == ".localhost") ||
      (host.size() >= 9u && host.substr(host.size() - 9u) == ".internal") ||
      looks_like_private_ipv4(host)) {
    raise_error("VENOM-E7000", "remote script URL targets a private or local host: " + host);
  }
}

std::string integrity_status_for(const std::string& declared_integrity,
                                 const std::vector<unsigned char>& bytes,
                                 const std::string& url) {
  if (declared_integrity.empty()) return "not-declared";

  const auto expected_sha256 = venom::package::sha256_sri(bytes);
  const auto expected_sha384 = venom::package::sha384_sri(bytes);
  const auto expected_sha512 = venom::package::sha512_sri(bytes);
  std::stringstream input(declared_integrity);
  std::string token;
  bool saw_supported = false;
  bool matched_sha256 = false;
  bool matched_sha384 = false;
  bool matched_sha512 = false;
  while (input >> token) {
    if (token.rfind("sha256-", 0) == 0) {
      saw_supported = true;
      matched_sha256 = matched_sha256 || token == expected_sha256;
    } else if (token.rfind("sha384-", 0) == 0) {
      saw_supported = true;
      matched_sha384 = matched_sha384 || token == expected_sha384;
    } else if (token.rfind("sha512-", 0) == 0) {
      saw_supported = true;
      matched_sha512 = matched_sha512 || token == expected_sha512;
    }
  }
  if (matched_sha512) return "verified-sha512";
  if (matched_sha384) return "verified-sha384";
  if (matched_sha256) return "verified-sha256";
  if (saw_supported) {
    raise_error("VENOM-E7000", "remote script SRI integrity mismatch: " + url);
  }
  raise_error("VENOM-E7000", "remote script declares no supported SRI algorithm (sha256/sha384/sha512): " + url);
}

std::filesystem::path default_cache_directory() {
  return std::filesystem::current_path() / ".venom-cache" / "remote";
}

std::string cache_metadata(const std::string& url,
                           const std::string& digest,
                           std::size_t byte_size) {
  std::ostringstream out;
  out << "VENOM_REMOTE_VENDOR_CACHE_V1\n"
      << "version=1\n"
      << "url=" << url << "\n"
      << "sha256=" << digest << "\n"
      << "bytes=" << byte_size << "\n";
  return out.str();
}

bool try_read_cache(const std::filesystem::path& body_path,
                    const std::filesystem::path& metadata_path,
                    const std::string& url,
                    std::size_t max_script_bytes,
                    std::vector<unsigned char>& bytes) {
  if (!std::filesystem::is_regular_file(body_path) || !std::filesystem::is_regular_file(metadata_path)) {
    return false;
  }
  const auto metadata = read_text(metadata_path);
  if (metadata.rfind("VENOM_REMOTE_VENDOR_CACHE_V1", 0) != 0 || metadata_value(metadata, "url") != url) {
    return false;
  }
  bytes = read_bytes(body_path);
  validate_script_bytes(bytes, max_script_bytes, url);
  const auto digest = venom::package::sha256_hex(bytes);
  if (metadata_value(metadata, "sha256") != digest || metadata_value(metadata, "bytes") != std::to_string(bytes.size())) {
    return false;
  }
  return true;
}

std::string downloader_executable() {
  const char* override_path = std::getenv("VENOM_CURL");
  if (override_path != nullptr && *override_path != '\0') {
    return std::string(override_path);
  }
#ifdef _WIN32
  // Resolve an actual executable instead of a PowerShell alias or command shim.
  return "curl.exe";
#else
  return "curl";
#endif
}

std::filesystem::path unique_temporary_path(const std::filesystem::path& directory,
                                            const std::string& key) {
  std::random_device random;
  std::ostringstream suffix;
  suffix << std::hex << random() << random();
  return directory / (key + "." + suffix.str() + ".download");
}

const RemoteVendorLockEntry* find_lock_entry(const RemoteVendorOptions& options,
                                              const std::string& normalized_url) {
  for (const auto& entry : options.lock_entries) {
    if (entry.normalized_url == normalized_url) return &entry;
  }
  return nullptr;
}

std::vector<RemoteVendorLockEntry> canonical_lock_entries(const std::vector<RemoteVendorRecord>& records) {
  std::map<std::string, RemoteVendorLockEntry> unique;
  for (const auto& record : records) {
    RemoteVendorLockEntry entry;
    entry.normalized_url = record.normalized_url;
    entry.content_sha256 = record.content_sha256;
    entry.integrity_status = record.integrity_status;
    entry.byte_size = record.byte_size;
    const auto inserted = unique.emplace(entry.normalized_url, entry);
    if (!inserted.second) {
      const auto& existing = inserted.first->second;
      if (existing.content_sha256 != entry.content_sha256 || existing.byte_size != entry.byte_size ||
          existing.integrity_status != entry.integrity_status) {
        raise_error("VENOM-E7000", "conflicting remote dependency records for: " + entry.normalized_url);
      }
    }
  }
  std::vector<RemoteVendorLockEntry> result;
  result.reserve(unique.size());
  for (const auto& item : unique) result.push_back(item.second);
  return result;
}

std::string lock_text_from_entries(const std::vector<RemoteVendorLockEntry>& entries) {
  std::ostringstream out;
  out << "VENOM_VENDOR_LOCK_V1\n"
      << "version=1\n"
      << "entry_count=" << entries.size() << "\n"
      << "url\tsha256\tbytes\tintegrity\n";
  for (const auto& entry : entries) {
    out << entry.normalized_url << '\t'
        << entry.content_sha256 << '\t'
        << entry.byte_size << '\t'
        << entry.integrity_status << '\n';
  }
  return out.str();
}

} // namespace

std::string normalize_remote_script_url(const std::string& url) {
  std::string normalized = url;
  if (normalized.rfind("//", 0) == 0) {
    normalized = "https:" + normalized;
  }
  const auto fragment = normalized.find('#');
  if (fragment != std::string::npos) {
    normalized.resize(fragment);
  }
  validate_public_https_url(normalized);
  return normalized;
}

RemoteVendorLock load_remote_vendor_lock(const std::filesystem::path& path) {
  RemoteVendorLock lock;
  lock.path = path;
  if (!std::filesystem::exists(path)) return lock;
  if (!std::filesystem::is_regular_file(path)) {
    raise_error("VENOM-E7000", "vendor lock path is not a regular file: " + path.string());
  }
  const auto text = read_text(path);
  std::stringstream input(text);
  std::string line;
  if (!std::getline(input, line)) {
    raise_error("VENOM-E7000", "vendor lock has an invalid header: " + path.string());
  }
  if (!line.empty() && line.back() == '\r') line.pop_back();
  if (line != "VENOM_VENDOR_LOCK_V1") {
    raise_error("VENOM-E7000", "vendor lock has an invalid header: " + path.string());
  }
  std::string version_line;
  std::string count_line;
  std::string columns_line;
  if (!std::getline(input, version_line) || !std::getline(input, count_line) || !std::getline(input, columns_line)) {
    raise_error("VENOM-E7000", "vendor lock is truncated: " + path.string());
  }
  for (auto* value : {&version_line, &count_line, &columns_line}) {
    if (!value->empty() && value->back() == '\r') value->pop_back();
  }
  if (version_line != "version=1" || columns_line != "url\tsha256\tbytes\tintegrity") {
    raise_error("VENOM-E7000", "vendor lock schema is unsupported: " + path.string());
  }
  if (count_line.rfind("entry_count=", 0) != 0) {
    raise_error("VENOM-E7000", "vendor lock is missing entry_count: " + path.string());
  }
  const auto expected_count = parse_size_value(count_line.substr(12), "vendor lock entry_count");
  std::set<std::string> seen;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line.empty()) continue;
    const auto fields = split_tabs(line);
    if (fields.size() != 4u) {
      raise_error("VENOM-E7000", "vendor lock entry has an invalid field count: " + path.string());
    }
    RemoteVendorLockEntry entry;
    entry.normalized_url = normalize_remote_script_url(fields[0]);
    if (entry.normalized_url != fields[0]) {
      raise_error("VENOM-E7000", "vendor lock URL is not normalized: " + fields[0]);
    }
    entry.content_sha256 = fields[1];
    if (!is_hex_digest(entry.content_sha256, 64u)) {
      raise_error("VENOM-E7000", "vendor lock has an invalid SHA-256 digest: " + entry.normalized_url);
    }
    entry.byte_size = parse_size_value(fields[2], "vendor lock byte size");
    if (entry.byte_size == 0u) {
      raise_error("VENOM-E7000", "vendor lock byte size must be non-zero: " + entry.normalized_url);
    }
    entry.integrity_status = fields[3];
    if (entry.integrity_status != "not-declared" &&
        entry.integrity_status != "verified-sha256" &&
        entry.integrity_status != "verified-sha384" &&
        entry.integrity_status != "verified-sha512") {
      raise_error("VENOM-E7000", "vendor lock has an invalid integrity status: " + entry.normalized_url);
    }
    if (!seen.insert(entry.normalized_url).second) {
      raise_error("VENOM-E7000", "vendor lock contains a duplicate URL: " + entry.normalized_url);
    }
    lock.entries.push_back(std::move(entry));
  }
  if (lock.entries.size() != expected_count) {
    raise_error("VENOM-E7000", "vendor lock entry_count does not match its records: " + path.string());
  }
  std::sort(lock.entries.begin(), lock.entries.end(), [](const auto& a, const auto& b) {
    return a.normalized_url < b.normalized_url;
  });
  lock.present = true;
  lock.canonical_sha256 = venom::package::sha256_hex(bytes_from_string(lock_text_from_entries(lock.entries)));
  return lock;
}

std::string make_remote_vendor_lock_text(const std::vector<RemoteVendorRecord>& records) {
  return lock_text_from_entries(canonical_lock_entries(records));
}

std::string remote_vendor_lock_sha256(const std::vector<RemoteVendorRecord>& records) {
  return venom::package::sha256_hex(bytes_from_string(make_remote_vendor_lock_text(records)));
}

void validate_remote_vendor_lock_coverage(const RemoteVendorLock& lock,
                                          const std::vector<RemoteVendorRecord>& records,
                                          bool refresh) {
  if (!lock.present || refresh) return;
  const auto actual = canonical_lock_entries(records);
  if (actual.size() != lock.entries.size()) {
    raise_error("VENOM-E7000", "venom.lock dependency count mismatch; run with --refresh-vendors to update the lock");
  }
  for (std::size_t i = 0; i < actual.size(); ++i) {
    const auto& expected = lock.entries[i];
    const auto& observed = actual[i];
    if (expected.normalized_url != observed.normalized_url) {
      raise_error("VENOM-E7000", "venom.lock dependency set mismatch; run with --refresh-vendors to update the lock");
    }
    if (expected.content_sha256 != observed.content_sha256 || expected.byte_size != observed.byte_size ||
        expected.integrity_status != observed.integrity_status) {
      raise_error("VENOM-E7000", "venom.lock record mismatch for: " + observed.normalized_url);
    }
  }
}

void write_remote_vendor_lock(const std::filesystem::path& path,
                              const std::vector<RemoteVendorRecord>& records) {
  write_text_atomic(path, make_remote_vendor_lock_text(records));
}

RemoteVendorResult vendor_remote_script(const std::string& source_url,
                                        const std::string& declared_integrity,
                                        const RemoteVendorOptions& options) {
  if (!options.enabled) {
    raise_error("VENOM-E7000", "production remote script vendoring is disabled");
  }
  const auto normalized_url = normalize_remote_script_url(source_url);
  const auto* locked = find_lock_entry(options, normalized_url);
  if (options.lock_present && !options.refresh && locked == nullptr) {
    raise_error("VENOM-E7000", "remote dependency is not present in venom.lock: " + normalized_url + "; run with --refresh-vendors to update the lock");
  }

  const auto cache_directory = options.cache_directory.empty() ? default_cache_directory() : options.cache_directory;
  std::filesystem::create_directories(cache_directory);
  const auto key = venom::package::sha256_hex(bytes_from_string(normalized_url));
  const auto body_path = cache_directory / (key + ".js");
  const auto metadata_path = cache_directory / (key + ".meta");

  RemoteVendorResult result;
  result.record.source_url = source_url;
  result.record.normalized_url = normalized_url;
  result.record.declared_integrity = declared_integrity;
  bool downloaded = false;

  if (!options.refresh && try_read_cache(body_path, metadata_path, normalized_url, options.max_script_bytes, result.bytes)) {
    result.record.cache_hit = true;
  } else {
    if (options.offline) {
      raise_error("VENOM-E7000", "remote vendor cache miss in offline mode: " + normalized_url);
    }
    const auto temporary = unique_temporary_path(cache_directory, key);
    const auto error_output = std::filesystem::path(temporary.string() + ".stderr");
    auto cleanup_temporary = [&]() {
      std::error_code cleanup_error;
      std::filesystem::remove(temporary, cleanup_error);
      cleanup_error.clear();
      std::filesystem::remove(error_output, cleanup_error);
    };
    cleanup_temporary();

    // The URL has already been validated as public HTTPS. Do not use curl's
    // --proto restriction here: on Windows it can reject an HTTP CONNECT proxy
    // used to transport an HTTPS request. Redirect following is enabled with a bounded HTTPS-only budget.
    // CDNs commonly redirect bootstrap URLs; content is still pinned by SRI/lock metadata.
    const std::vector<std::string> arguments{
      "--disable",
      "--tlsv1.2",
      "--fail",
      "--silent",
      "--show-error",
      "--compressed",
      "--location",
      "--connect-timeout", "10",
      "--max-time", "90",
      "--proto-redir", "=https",
      "--max-redirs", "8",
      "--max-filesize", std::to_string(options.max_script_bytes),
      "--user-agent", VENOM_REMOTE_VENDOR_USER_AGENT,
      "--stderr", error_output.string(),
      "--output", temporary.string(),
      normalized_url,
    };
    const auto exit_code = run_process(downloader_executable(), arguments);
    if (exit_code != 0) {
      std::string detail = read_text(error_output);
      if (detail.size() > 2048u) detail.resize(2048u);
      cleanup_temporary();
      std::string message = "failed to vendor remote script (curl exit " +
                            std::to_string(exit_code) + "): " + normalized_url;
      if (!detail.empty()) message += "\n" + detail;
      raise_error("VENOM-E7000", message);
    }
    if (!std::filesystem::is_regular_file(temporary)) {
      std::string detail = read_text(error_output);
      if (detail.size() > 2048u) detail.resize(2048u);
      cleanup_temporary();
      std::string message = "remote downloader completed without producing an output file: " + normalized_url;
      if (!detail.empty()) message += "\n" + detail;
      raise_error("VENOM-E7000", message);
    }
    std::error_code size_error;
    const auto downloaded_size = std::filesystem::file_size(temporary, size_error);
    if (size_error) {
      cleanup_temporary();
      raise_error("VENOM-E7000", "failed to inspect downloaded remote vendor file: " + normalized_url);
    }
    if (downloaded_size > options.max_script_bytes) {
      cleanup_temporary();
      raise_error("VENOM-E7000", "remote script exceeds the production vendor size limit: " + normalized_url);
    }
    try {
      result.bytes = read_bytes(temporary);
    } catch (...) {
      cleanup_temporary();
      throw;
    }
    cleanup_temporary();
    downloaded = true;
  }

  validate_script_bytes(result.bytes, options.max_script_bytes, normalized_url);
  result.record.content_sha256 = venom::package::sha256_hex(result.bytes);
  result.record.integrity_status = integrity_status_for(declared_integrity, result.bytes, normalized_url);
  result.record.byte_size = result.bytes.size();

  if (locked != nullptr && !options.refresh) {
    if (locked->content_sha256 != result.record.content_sha256 ||
        locked->byte_size != result.record.byte_size ||
        locked->integrity_status != result.record.integrity_status) {
      raise_error("VENOM-E7000", "venom.lock rejected changed remote dependency: " + normalized_url + "; run with --refresh-vendors only after reviewing the dependency update");
    }
    result.record.lock_match = true;
  }

  // Commit a new cache body only after SRI and lock verification succeed. A changed
  // unreviewed dependency therefore cannot overwrite the last pinned cache entry.
  if (downloaded) {
    write_bytes_atomic(body_path, result.bytes);
    write_text_atomic(metadata_path, cache_metadata(normalized_url, result.record.content_sha256, result.bytes.size()));
  }
  return result;
}

std::string make_remote_vendor_metadata(const std::vector<RemoteVendorRecord>& records,
                                        bool offline,
                                        const std::filesystem::path& cache_directory,
                                        const std::filesystem::path& lock_file,
                                        const std::string& lock_sha256,
                                        const std::string& lock_mode) {
  std::ostringstream out;
  std::size_t cache_hits = 0;
  std::size_t lock_matches = 0;
  for (const auto& record : records) {
    if (record.cache_hit) ++cache_hits;
    if (record.lock_match) ++lock_matches;
  }
  const auto unique_entries = canonical_lock_entries(records);
  out << "VENOM_REMOTE_VENDOR_V2\n"
      << "version=2\n"
      << "enabled=true\n"
      << "policy=build-time-fetch-package-bytecode\n"
      << "network_runtime_required=false\n"
      << "unvendored_remote_scripts=0\n"
      << "vendor_count=" << records.size() << "\n"
      << "unique_vendor_count=" << unique_entries.size() << "\n"
      << "cache_hits=" << cache_hits << "\n"
      << "offline=" << (offline ? "true" : "false") << "\n"
      << "cache_location=build-local-not-shipped\n"
      << "vendor_lock_required=true\n"
      << "vendor_lock_mode=" << lock_mode << "\n"
      << "vendor_lock_entries=" << unique_entries.size() << "\n"
      << "vendor_lock_matches=" << lock_matches << "\n"
      << "vendor_lock_sha256=" << lock_sha256 << "\n"
      << "vendor_lock_location=source-local-not-shipped\n";
  (void)cache_directory;
  (void)lock_file;
  for (std::size_t i = 0; i < records.size(); ++i) {
    const auto& record = records[i];
    out << "vendor\t" << i << "\t" << record.normalized_url
        << "\tsha256=" << record.content_sha256
        << "\tbytes=" << record.byte_size
        << "\tcache=" << (record.cache_hit ? "hit" : "miss")
        << "\tlock=" << (record.lock_match ? "match" : lock_mode)
        << "\tintegrity=" << record.integrity_status << "\n";
  }
  return out.str();
}

} // namespace venom::compiler

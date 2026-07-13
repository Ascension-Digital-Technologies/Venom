#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace venom::compiler {

struct RemoteVendorLockEntry {
  std::string normalized_url;
  std::string content_sha256;
  std::string integrity_status;
  std::size_t byte_size = 0;
};

struct RemoteVendorLock {
  bool present = false;
  std::filesystem::path path;
  std::vector<RemoteVendorLockEntry> entries;
  std::string canonical_sha256;
};

struct RemoteVendorOptions {
  bool enabled = true;
  bool offline = false;
  bool refresh = false;
  std::filesystem::path cache_directory;
  std::filesystem::path lock_file;
  bool lock_present = false;
  std::vector<RemoteVendorLockEntry> lock_entries;
  std::size_t max_script_bytes = 8u * 1024u * 1024u;
  std::string bridge_id_salt;
};

struct RemoteVendorRecord {
  std::string source_url;
  std::string normalized_url;
  std::string content_sha256;
  std::string declared_integrity;
  std::string integrity_status;
  std::size_t byte_size = 0;
  bool cache_hit = false;
  bool lock_match = false;
};

struct RemoteVendorResult {
  std::vector<unsigned char> bytes;
  RemoteVendorRecord record;
};

std::string normalize_remote_script_url(const std::string& url);
RemoteVendorLock load_remote_vendor_lock(const std::filesystem::path& path);
std::string make_remote_vendor_lock_text(const std::vector<RemoteVendorRecord>& records);
std::string remote_vendor_lock_sha256(const std::vector<RemoteVendorRecord>& records);
void validate_remote_vendor_lock_coverage(const RemoteVendorLock& lock,
                                          const std::vector<RemoteVendorRecord>& records,
                                          bool refresh);
void write_remote_vendor_lock(const std::filesystem::path& path,
                              const std::vector<RemoteVendorRecord>& records);
RemoteVendorResult vendor_remote_script(const std::string& source_url,
                                        const std::string& declared_integrity,
                                        const RemoteVendorOptions& options);
std::string make_remote_vendor_metadata(const std::vector<RemoteVendorRecord>& records,
                                        bool offline,
                                        const std::filesystem::path& cache_directory,
                                        const std::filesystem::path& lock_file,
                                        const std::string& lock_sha256,
                                        const std::string& lock_mode);

} // namespace venom::compiler

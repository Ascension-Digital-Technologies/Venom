#include "base/error.hpp"
#include "turbojs/bytecode.hpp"

#include "package/hash.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <mutex>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <atomic>

#if defined(VENOM_ENABLE_FULL_TURBOJS)
#include "turbojs.h"
#endif

namespace venom::turbojs {
namespace {

constexpr unsigned char kNativeMagic[8] = {'V','T','J','S','B','C','0','3'};
constexpr std::uint32_t kNativeRecordVersion = 3u;
constexpr std::uint32_t kNativeHeaderSize = 48u;
constexpr std::uint32_t kFlagModule = 1u << 0u;
constexpr std::uint32_t kTurboJsBytecodeAbi = 0x01000300u;

std::mutex g_bytecode_cache_mutex;
bool g_bytecode_cache_enabled = false;
bool g_bytecode_cache_verbose = false;
std::filesystem::path g_bytecode_cache_directory;
std::atomic<std::size_t> g_bytecode_cache_hits{0};
std::atomic<std::size_t> g_bytecode_cache_misses{0};
std::atomic<std::size_t> g_bytecode_cache_writes{0};

std::vector<unsigned char> read_binary_if_present(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  return std::vector<unsigned char>(std::istreambuf_iterator<char>(in), {});
}

void write_binary_atomic(const std::filesystem::path& path, const std::vector<unsigned char>& content) {
  std::filesystem::create_directories(path.parent_path());
  const auto temporary = path.string() + ".tmp";
  {
    std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
    if (!out) raise_error("VENOM-E6000", "failed to write TurboJS bytecode cache entry " + temporary);
    out.write(reinterpret_cast<const char*>(content.data()), static_cast<std::streamsize>(content.size()));
  }
  std::error_code error;
  std::filesystem::rename(temporary, path, error);
  if (error) {
    std::filesystem::remove(path, error);
    error.clear();
    std::filesystem::rename(temporary, path, error);
  }
  if (error) {
    std::filesystem::remove(temporary);
    raise_error("VENOM-E6000", "failed to publish TurboJS bytecode cache entry " + path.string());
  }
}

std::string bytecode_cache_key(const std::string& source,
                               const std::string& source_name,
                               bool is_module,
                               bool redact_source_metadata,
                               const std::vector<ModuleSourceRecord>* module_sources) {
  std::string material = "venom-tjs-bytecode-cache-v1\nabi=" + std::to_string(kTurboJsBytecodeAbi) +
      "\nmodule=" + std::string(is_module ? "1" : "0") +
      "\nredact=" + std::string(redact_source_metadata ? "1" : "0") +
      "\nname=" + source_name + "\nsource=" + source;
  if (module_sources) {
    material += "\nmodule-count=" + std::to_string(module_sources->size());
    for (const auto& record : *module_sources) {
      material += "\nsource-name=" + record.source_name +
                  "\ncompile-name=" + record.compile_name +
                  "\nmodule-source=" + record.source;
    }
  }
  return venom::package::sha256_hex(
      reinterpret_cast<const unsigned char*>(material.data()), material.size());
}

void append_u32(std::vector<unsigned char>& out, std::uint32_t value) {
  out.push_back(static_cast<unsigned char>(value & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 8u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 16u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 24u) & 0xffu));
}

void append_u64(std::vector<unsigned char>& out, std::uint64_t value) {
  for (int i = 0; i < 8; ++i) {
    out.push_back(static_cast<unsigned char>((value >> (i * 8)) & 0xffu));
  }
}

std::vector<unsigned char> source_bytes_for_hash(const std::string& source) {
  return std::vector<unsigned char>(source.begin(), source.end());
}

#if defined(VENOM_ENABLE_FULL_TURBOJS)
std::string take_exception(JSContext* ctx, const std::string& source_name) {
  JSValue ex = JS_GetException(ctx);
  const char* text = JS_ToCString(ctx, ex);
  std::string message = text ? text : "TurboJS compilation failed";
  if (text) JS_FreeCString(ctx, text);
  JS_FreeValue(ctx, ex);
  return "TurboJS compile failed for " + source_name + ": " + message;
}

std::string normalize_module_path(const std::string& base_name, const std::string& requested_name) {
  std::string joined;
  if (requested_name.rfind("./", 0) == 0 || requested_name.rfind("../", 0) == 0) {
    const auto slash = base_name.find_last_of("/\\");
    joined = (slash == std::string::npos ? std::string{} : base_name.substr(0, slash + 1)) + requested_name;
  } else {
    joined = requested_name;
  }
  std::replace(joined.begin(), joined.end(), '\\', '/');
  std::vector<std::string> parts;
  std::size_t cursor = 0;
  while (cursor <= joined.size()) {
    const auto next = joined.find('/', cursor);
    const auto part = joined.substr(cursor, next == std::string::npos ? std::string::npos : next - cursor);
    if (part.empty() || part == ".") {
      // Skip.
    } else if (part == "..") {
      if (!parts.empty()) parts.pop_back();
    } else {
      parts.push_back(part);
    }
    if (next == std::string::npos) break;
    cursor = next + 1;
  }
  std::ostringstream out;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i) out << '/';
    out << parts[i];
  }
  return out.str();
}

struct ModuleLoaderState {
  std::unordered_map<std::string, std::string> source_by_original;
  std::unordered_map<std::string, std::string> compile_by_original;
  std::unordered_map<std::string, std::string> original_by_compile;
};

char* normalize_module_name(JSContext* ctx,
                            const char* module_base_name,
                            const char* module_name,
                            void* opaque) {
  auto* state = static_cast<ModuleLoaderState*>(opaque);
  if (!state || !module_name) return nullptr;
  if (const auto direct = state->original_by_compile.find(module_name); direct != state->original_by_compile.end()) {
    const std::string resolved = module_name;
    char* result = static_cast<char*>(js_malloc(ctx, resolved.size() + 1u));
    if (!result) return nullptr;
    std::memcpy(result, resolved.c_str(), resolved.size() + 1u);
    return result;
  }
  std::string base = module_base_name ? module_base_name : "";
  if (const auto it = state->original_by_compile.find(base); it != state->original_by_compile.end()) {
    base = it->second;
  }
  const auto original = normalize_module_path(base, module_name);
  auto mapped = state->compile_by_original.find(original);
  if (mapped == state->compile_by_original.end()) {
    for (const auto* extension : {".js", ".mjs", ".cjs", ".jsx", ".ts", ".tsx", ".mts", ".cts"}) {
      mapped = state->compile_by_original.find(original + extension);
      if (mapped != state->compile_by_original.end()) break;
    }
  }
  if (mapped == state->compile_by_original.end()) {
    for (const auto* extension : {".js", ".mjs", ".jsx", ".ts", ".tsx", ".mts", ".cts"}) {
      mapped = state->compile_by_original.find(original + "/index" + extension);
      if (mapped != state->compile_by_original.end()) break;
    }
  }
  if (mapped == state->compile_by_original.end()) {
    auto without_known_extension = [](std::string value) {
      for (const auto* extension : {".js", ".mjs", ".cjs", ".jsx", ".ts", ".tsx", ".mts", ".cts"}) {
        const std::string ext(extension);
        if (value.size() >= ext.size() && value.compare(value.size() - ext.size(), ext.size(), ext) == 0) {
          value.resize(value.size() - ext.size());
          break;
        }
      }
      return value;
    };
    const auto extensionless = without_known_extension(original);
    for (auto it = state->compile_by_original.begin(); it != state->compile_by_original.end(); ++it) {
      const auto candidate = without_known_extension(normalize_module_path("", it->first));
      const auto requested_slash = extensionless.find_last_of('/');
      const auto candidate_slash = candidate.find_last_of('/');
      const auto requested_basename = extensionless.substr(requested_slash == std::string::npos ? 0u : requested_slash + 1u);
      const auto candidate_basename = candidate.substr(candidate_slash == std::string::npos ? 0u : candidate_slash + 1u);
      if (candidate == extensionless || candidate == extensionless + "/index" || candidate_basename == requested_basename) {
        mapped = it;
        break;
      }
    }
  }
  if (mapped == state->compile_by_original.end()) {
    JS_ThrowReferenceError(ctx, "could not resolve protected module '%s' from '%s'", module_name, base.c_str());
    return nullptr;
  }
  const auto& resolved = mapped->second;
  char* result = static_cast<char*>(js_malloc(ctx, resolved.size() + 1u));
  if (!result) return nullptr;
  std::memcpy(result, resolved.c_str(), resolved.size() + 1u);
  return result;
}

JSModuleDef* load_module_source(JSContext* ctx, const char* module_name, void* opaque) {
  auto* state = static_cast<ModuleLoaderState*>(opaque);
  if (!state || !module_name) return nullptr;
  const auto original_it = state->original_by_compile.find(module_name);
  if (original_it == state->original_by_compile.end()) {
    JS_ThrowReferenceError(ctx, "protected module id not found: %s", module_name);
    return nullptr;
  }
  const auto source_it = state->source_by_original.find(original_it->second);
  if (source_it == state->source_by_original.end()) {
    JS_ThrowReferenceError(ctx, "protected module source not found: %s", module_name);
    return nullptr;
  }
  JSValue object = JS_Eval(ctx,
                           source_it->second.data(),
                           source_it->second.size(),
                           module_name,
                           JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY | JS_EVAL_FLAG_STRICT);
  if (JS_IsException(object)) return nullptr;
  if (JS_RewriteModuleRequestNames(ctx, object, original_it->second.c_str(), normalize_module_name, opaque) < 0) {
    JS_FreeValue(ctx, object);
    return nullptr;
  }
  auto* module = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(object));
  JS_FreeValue(ctx, object);
  return module;
}
#endif

} // namespace

BytecodeCacheStats bytecode_cache_stats() { return {g_bytecode_cache_hits.load(), g_bytecode_cache_misses.load(), g_bytecode_cache_writes.load()}; }
void reset_bytecode_cache_stats() { g_bytecode_cache_hits = 0; g_bytecode_cache_misses = 0; g_bytecode_cache_writes = 0; }

void configure_bytecode_cache(bool enabled, const std::filesystem::path& directory, bool verbose) {
  std::lock_guard<std::mutex> lock(g_bytecode_cache_mutex);
  g_bytecode_cache_enabled = enabled;
  g_bytecode_cache_verbose = verbose;
  g_bytecode_cache_directory = directory;
  if (enabled && !directory.empty()) std::filesystem::create_directories(directory);
}

std::vector<unsigned char> compile_protected_portable_bytecode(const std::string& source) {
  return compile_native_turbojs_bytecode(source, "<venom-script>", false);
}

std::vector<unsigned char> compile_native_turbojs_bytecode(const std::string& source,
                                                           const std::string& source_name,
                                                           bool is_module,
                                                           bool redact_source_metadata,
                                                           const std::vector<ModuleSourceRecord>* module_sources) {
#if !defined(VENOM_ENABLE_FULL_TURBOJS)
  (void)source;
  (void)source_name;
  (void)is_module;
  (void)redact_source_metadata;
  (void)module_sources;
  raise_error("VENOM-E6000", "production TurboJS bytecode generation requires VENOM_ENABLE_FULL_TURBOJS=ON");
#else
  std::filesystem::path cache_file;
  bool cache_enabled = false;
  bool cache_verbose = false;
  {
    std::lock_guard<std::mutex> lock(g_bytecode_cache_mutex);
    cache_enabled = g_bytecode_cache_enabled && !g_bytecode_cache_directory.empty();
    cache_verbose = g_bytecode_cache_verbose;
    if (cache_enabled) {
      const auto digest = bytecode_cache_key(source, source_name, is_module, redact_source_metadata, module_sources);
      cache_file = g_bytecode_cache_directory / digest.substr(0, 2) / (digest + ".vqbc");
      const auto cached = read_binary_if_present(cache_file);
      if (is_native_turbojs_bytecode(cached)) {
        ++g_bytecode_cache_hits;
        if (cache_verbose) std::cerr << "[CACHE]    TurboJS bytecode hit: " << source_name << "\n";
        return cached;
      }
      ++g_bytecode_cache_misses;
      if (cache_verbose) std::cerr << "[CACHE]    TurboJS bytecode miss: " << source_name << "\n";
    }
  }
  (void)redact_source_metadata; // Public manifests are redacted; VTJSBC03 integrity fields remain authenticated.
  JSRuntime* runtime = JS_NewRuntime();
  if (!runtime) raise_error("VENOM-E6000", "failed to create TurboJS compiler runtime");
  JS_SetMemoryLimit(runtime, 256u * 1024u * 1024u);
  JS_SetMaxStackSize(runtime, 8u * 1024u * 1024u);
  ModuleLoaderState module_state;
  if (is_module && module_sources) {
    for (const auto& record : *module_sources) {
      if (record.source_name.empty() || record.compile_name.empty()) continue;
      module_state.source_by_original[record.source_name] = record.source;
      module_state.compile_by_original[record.source_name] = record.compile_name;
      module_state.original_by_compile[record.compile_name] = record.source_name;
    }
    JS_SetModuleLoaderFunc(runtime, normalize_module_name, load_module_source, &module_state);
  }
  JSContext* context = JS_NewContext(runtime);
  if (!context) {
    JS_FreeRuntime(runtime);
    raise_error("VENOM-E6000", "failed to create TurboJS compiler context");
  }

  const int eval_flags = (is_module ? JS_EVAL_TYPE_MODULE : JS_EVAL_TYPE_GLOBAL) |
                         JS_EVAL_FLAG_COMPILE_ONLY |
                         JS_EVAL_FLAG_STRICT;
  JSValue object = JS_Eval(context,
                           source.data(),
                           source.size(),
                           source_name.empty() ? "<venom-script>" : source_name.c_str(),
                           eval_flags);
  if (JS_IsException(object)) {
    const auto message = take_exception(context, source_name.empty() ? "<venom-script>" : source_name);
    JS_FreeValue(context, object);
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    raise_error("VENOM-E6000", message);
  }
  if (is_module && module_sources) {
    std::string original_base = source_name;
    if (const auto it = module_state.original_by_compile.find(source_name); it != module_state.original_by_compile.end()) {
      original_base = it->second;
    }
    if (JS_RewriteModuleRequestNames(context, object, original_base.c_str(), normalize_module_name, &module_state) < 0) {
      const auto message = take_exception(context, source_name.empty() ? "<venom-module>" : source_name);
      JS_FreeValue(context, object);
      JS_FreeContext(context);
      JS_FreeRuntime(runtime);
      raise_error("VENOM-E6000", message);
    }
  }

  size_t bytecode_size = 0;
  const int write_flags = JS_WRITE_OBJ_BYTECODE |
                          JS_WRITE_OBJ_STRIP_SOURCE |
                          JS_WRITE_OBJ_STRIP_DEBUG;
  uint8_t* bytecode = JS_WriteObject(context, &bytecode_size, object, write_flags);
  JS_FreeValue(context, object);
  if (!bytecode || bytecode_size == 0u) {
    if (bytecode) js_free(context, bytecode);
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    raise_error("VENOM-E6000", "TurboJS emitted an empty bytecode object for " + source_name);
  }
  if (bytecode_size > 0xffffffffu || source.size() > 0xffffffffu) {
    js_free(context, bytecode);
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    raise_error("VENOM-E6000", "TurboJS bytecode record exceeds u32 limits for " + source_name);
  }

  const auto source_bytes = source_bytes_for_hash(source);
  const std::uint64_t source_hash = venom::package::fnv1a64(source_bytes);
  const std::uint64_t bytecode_hash = venom::package::fnv1a64(bytecode, bytecode_size);

  std::vector<unsigned char> out;
  out.reserve(kNativeHeaderSize + bytecode_size);
  out.insert(out.end(), std::begin(kNativeMagic), std::end(kNativeMagic));
  append_u32(out, kNativeRecordVersion);
  append_u32(out, is_module ? kFlagModule : 0u);
  append_u32(out, static_cast<std::uint32_t>(bytecode_size));
  append_u32(out, kTurboJsBytecodeAbi);
  append_u64(out, bytecode_hash);
  // VTJSBC03 authenticates these fields inside the executable record. Keep them
  // populated until a coordinated VTJSBC04 ABI removes source-derived fields;
  // only the public/text manifests are redacted in release builds.
  append_u64(out, source_hash);
  append_u32(out, static_cast<std::uint32_t>(source.size()));
  append_u32(out, kNativeHeaderSize);
  out.insert(out.end(), bytecode, bytecode + bytecode_size);

  js_free(context, bytecode);
  JS_FreeContext(context);
  JS_FreeRuntime(runtime);
  if (cache_enabled) {
    std::lock_guard<std::mutex> lock(g_bytecode_cache_mutex);
    write_binary_atomic(cache_file, out);
    ++g_bytecode_cache_writes;
  }
  return out;
#endif
}


std::vector<unsigned char> compile_native_turbojs_module_bundle(const std::string& entry_source_name,
                                                               const std::vector<ModuleSourceRecord>& module_sources,
                                                               bool redact_source_metadata) {
  if (module_sources.empty()) raise_error("VENOM-E6000", "TurboJS module bundle requires at least one module");
  if (module_sources.size() > 512u) raise_error("VENOM-E6000", "TurboJS module bundle exceeds 512 modules");

  struct PackedModule {
    std::string name;
    std::vector<unsigned char> record;
  };
  std::vector<PackedModule> modules;
  modules.reserve(module_sources.size());
  std::uint32_t entry_index = 0xffffffffu;
  for (std::size_t i = 0; i < module_sources.size(); ++i) {
    const auto& source = module_sources[i];
    if (source.compile_name.empty()) raise_error("VENOM-E6000", "TurboJS module bundle contains an empty module name");
    if (source.compile_name == entry_source_name || source.source_name == entry_source_name) {
      entry_index = static_cast<std::uint32_t>(i);
    }
    modules.push_back(PackedModule{
        source.compile_name,
        compile_native_turbojs_bytecode(source.source,
                                        source.compile_name,
                                        true,
                                        redact_source_metadata,
                                        &module_sources)});
  }
  if (entry_index == 0xffffffffu) raise_error("VENOM-E6000", "TurboJS module bundle entry was not found: " + entry_source_name);

  constexpr std::uint32_t header_size = 24u;
  constexpr std::uint32_t entry_size = 16u;
  const std::uint64_t table_end64 = static_cast<std::uint64_t>(header_size) +
                                    static_cast<std::uint64_t>(modules.size()) * entry_size;
  if (table_end64 > 0xffffffffu) raise_error("VENOM-E6000", "TurboJS module bundle table exceeds u32 limits");
  std::uint32_t cursor = static_cast<std::uint32_t>(table_end64);

  struct TableEntry {
    std::uint32_t name_offset;
    std::uint32_t name_size;
    std::uint32_t record_offset;
    std::uint32_t record_size;
  };
  std::vector<TableEntry> table;
  table.reserve(modules.size());
  for (const auto& module : modules) {
    if (module.name.size() > 0xffffffffu || module.record.size() > 0xffffffffu) {
      raise_error("VENOM-E6000", "TurboJS module bundle entry exceeds u32 limits");
    }
    const std::uint32_t name_size = static_cast<std::uint32_t>(module.name.size());
    const std::uint32_t record_size = static_cast<std::uint32_t>(module.record.size());
    const std::uint64_t record_offset64 = static_cast<std::uint64_t>(cursor) + name_size;
    const std::uint64_t next64 = record_offset64 + record_size;
    if (next64 > 0xffffffffu) raise_error("VENOM-E6000", "TurboJS module bundle exceeds u32 limits");
    table.push_back(TableEntry{cursor, name_size, static_cast<std::uint32_t>(record_offset64), record_size});
    cursor = static_cast<std::uint32_t>(next64);
  }

  std::vector<unsigned char> out;
  out.reserve(cursor);
  out.insert(out.end(), {'V','T','J','S','M','B','0','4'});
  append_u32(out, 1u);
  append_u32(out, static_cast<std::uint32_t>(modules.size()));
  append_u32(out, entry_index);
  append_u32(out, header_size);
  for (const auto& entry : table) {
    append_u32(out, entry.name_offset);
    append_u32(out, entry.name_size);
    append_u32(out, entry.record_offset);
    append_u32(out, entry.record_size);
  }
  for (std::size_t i = 0; i < modules.size(); ++i) {
    out.insert(out.end(), modules[i].name.begin(), modules[i].name.end());
    out.insert(out.end(), modules[i].record.begin(), modules[i].record.end());
  }
  if (out.size() != cursor) raise_error("VENOM-E6000", "TurboJS module bundle size accounting failed");
  return out;
}

bool is_protected_portable_bytecode(const std::vector<unsigned char>& bytes) {
  const char legacy_magic[] = {'V','T','J','S','B','C','0','2'};
  return bytes.size() >= sizeof(legacy_magic) &&
         std::equal(bytes.begin(), bytes.begin() + sizeof(legacy_magic), legacy_magic);
}

bool is_native_turbojs_bytecode(const std::vector<unsigned char>& bytes) {
  return bytes.size() >= sizeof(kNativeMagic) &&
         std::equal(bytes.begin(), bytes.begin() + sizeof(kNativeMagic), std::begin(kNativeMagic));
}

std::string bytecode_format_name(BytecodeFormat format) {
  switch (format) {
    case BytecodeFormat::SourcePreservingV1: return "source-preserving-byte-buffer-record";
    case BytecodeFormat::ProtectedPortableV2: return "legacy-source-masked-record-rejected";
    case BytecodeFormat::NativeTurboJsObjectV1: return "native-turbojs-object-bytecode-v1";
    case BytecodeFormat::NativeTurboJsObjectV3: return "native-turbojs-object-bytecode-v3";
  }
  return "unknown";
}

std::string bytecode_manifest_text(const std::vector<BytecodeChunkRecord>& chunks,
                                   std::uint32_t runtime_abi,
                                   std::uint32_t package_version) {
  std::ostringstream out;
  out << "VENOM_TJS_BYTECODE_MANIFEST_V3\n"
      << "version=3\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "format=native-turbojs-object-bytecode-v3\n"
      << "magic=VTJSBC03\n"
      << "writer=JS_WriteObject\n"
      << "reader=JS_ReadObject\n"
      << "write_flags=BYTECODE|STRIP_SOURCE|STRIP_DEBUG\n"
      << "exec_claim=wasm-turbojs-reads-and-evaluates-object-bytecode\n"
      << "cache_policy=route-scoped-bytecode-hash\n"
      << "source_preserving=false\n"
      << "legacy_vtjsbc01_allowed=false\n"
      << "legacy_vtjsbc02_allowed=false\n"
      << "record_fields=order|route|source|source_bytes|source_hash|bytecode_bytes|format\n"
      << "chunk_count=" << chunks.size() << "\n";
  for (const auto& chunk : chunks) {
    out << "bytecode_chunk\t" << chunk.order << "\t" << chunk.route << "\t" << chunk.source
        << "\tsource_bytes=" << chunk.source_size
        << "\tsource_hash=0x" << std::hex << chunk.source_hash << std::dec
        << "\tbytecode_bytes=" << chunk.bytecode_size
        << "\tformat=" << bytecode_format_name(chunk.format) << "\n";
  }
  return out.str();
}

std::string module_resolver_metadata_text(std::uint32_t runtime_abi,
                                          std::uint32_t package_version,
                                          std::size_t chunk_count) {
  std::ostringstream out;
  out << "VENOM_TJS_MODULE_RESOLVER_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "enabled=true\n"
      << "resolver_abi=1\n"
      << "mode=package-relative-module-map\n"
      << "resolve_export=venom_tjs_module_resolve\n"
      << "module_record_ptr=venom_tjs_module_record_ptr\n"
      << "module_record_size=venom_tjs_module_record_size\n"
      << "deny_remote_modules=true\n"
      << "literal_dynamic_import_bundle=VTJSMB04\n"
      << "chunk_count=" << chunk_count << "\n";
  return out.str();
}

std::string exception_abi_metadata_text(std::uint32_t runtime_abi,
                                        std::uint32_t package_version) {
  std::ostringstream out;
  out << "VENOM_TJS_EXCEPTION_ABI_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "enabled=true\n"
      << "exception_abi=1\n"
      << "exception_ptr=venom_tjs_exception_ptr\n"
      << "exception_size=venom_tjs_exception_size\n"
      << "exception_code=venom_tjs_exception_code\n"
      << "exception_clear=venom_tjs_exception_clear\n"
      << "schema=ok|abi|context|code|kind|message|sourceBytes|sourceHash\n";
  return out.str();
}

std::string host_trap_policy_metadata_text(bool strict_release,
                                           std::uint32_t runtime_abi,
                                           std::uint32_t package_version) {
  std::ostringstream out;
  out << "VENOM_TJS_HOST_TRAP_POLICY_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "enabled=true\n"
      << "policy=" << (strict_release ? "deny-unknown-host-imports" : "record-unknown-host-imports") << "\n"
      << "unknown_import=" << (strict_release ? "trap" : "record-and-fallback") << "\n"
      << "host_trap_policy_ptr=venom_tjs_host_trap_policy_ptr\n"
      << "host_trap_policy_size=venom_tjs_host_trap_policy_size\n"
      << "host_trap_policy_hash=venom_tjs_host_trap_policy_hash\n"
      << "known_import_table=turbojs-host-imports.vqhi\n";
  return out.str();
}

} // namespace venom::turbojs

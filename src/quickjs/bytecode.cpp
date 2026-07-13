#include "quickjs/bytecode.hpp"

#include "package/hash.hpp"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#if defined(VENOM_ENABLE_FULL_QUICKJS)
#include "quickjs.h"
#endif

namespace venom::quickjs {
namespace {

constexpr unsigned char kNativeMagic[8] = {'V','Q','J','S','B','C','0','3'};
constexpr std::uint32_t kNativeRecordVersion = 3u;
constexpr std::uint32_t kNativeHeaderSize = 48u;
constexpr std::uint32_t kFlagModule = 1u << 0u;
constexpr std::uint32_t kQuickJsBytecodeAbi = 0x01000300u;

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

#if defined(VENOM_ENABLE_FULL_QUICKJS)
std::string take_exception(JSContext* ctx, const std::string& source_name) {
  JSValue ex = JS_GetException(ctx);
  const char* text = JS_ToCString(ctx, ex);
  std::string message = text ? text : "QuickJS compilation failed";
  if (text) JS_FreeCString(ctx, text);
  JS_FreeValue(ctx, ex);
  return "QuickJS compile failed for " + source_name + ": " + message;
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
  const auto mapped = state->compile_by_original.find(original);
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

std::vector<unsigned char> compile_placeholder_bytecode(const std::string& source) {
  return compile_native_quickjs_bytecode(source, "<venom-script>", false);
}

std::vector<unsigned char> compile_byte_buffer_record(const std::string& source) {
  // Kept only for debug/backward compatibility tests. Production call sites use
  // compile_native_quickjs_bytecode and release checks reject VQJSBC01.
  std::vector<unsigned char> out;
  const char magic[] = {'V','Q','J','S','B','C','0','1'};
  out.insert(out.end(), magic, magic + sizeof(magic));
  const std::uint32_t version = 1;
  const std::uint32_t size = static_cast<std::uint32_t>(source.size());
  const std::uint64_t hash = venom::package::fnv1a64(source_bytes_for_hash(source));
  append_u32(out, version);
  append_u32(out, size);
  append_u64(out, hash);
  out.insert(out.end(), source.begin(), source.end());
  return out;
}

std::vector<unsigned char> compile_protected_portable_bytecode(const std::string& source) {
  return compile_native_quickjs_bytecode(source, "<venom-script>", false);
}

std::vector<unsigned char> compile_native_quickjs_bytecode(const std::string& source,
                                                           const std::string& source_name,
                                                           bool is_module,
                                                           bool redact_source_metadata,
                                                           const std::vector<ModuleSourceRecord>* module_sources) {
#if !defined(VENOM_ENABLE_FULL_QUICKJS)
  (void)source;
  (void)source_name;
  (void)is_module;
  (void)redact_source_metadata;
  (void)module_sources;
  throw std::runtime_error("production QuickJS bytecode generation requires VENOM_ENABLE_FULL_QUICKJS=ON");
#else
  (void)redact_source_metadata; // Public manifests are redacted; VQJSBC03 integrity fields remain authenticated.
  JSRuntime* runtime = JS_NewRuntime();
  if (!runtime) throw std::runtime_error("failed to create QuickJS compiler runtime");
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
    throw std::runtime_error("failed to create QuickJS compiler context");
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
    throw std::runtime_error(message);
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
      throw std::runtime_error(message);
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
    throw std::runtime_error("QuickJS emitted an empty bytecode object for " + source_name);
  }
  if (bytecode_size > 0xffffffffu || source.size() > 0xffffffffu) {
    js_free(context, bytecode);
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    throw std::runtime_error("QuickJS bytecode record exceeds u32 limits for " + source_name);
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
  append_u32(out, kQuickJsBytecodeAbi);
  append_u64(out, bytecode_hash);
  // VQJSBC03 authenticates these fields inside the executable record. Keep them
  // populated until a coordinated VQJSBC04 ABI removes source-derived fields;
  // only the public/text manifests are redacted in release builds.
  append_u64(out, source_hash);
  append_u32(out, static_cast<std::uint32_t>(source.size()));
  append_u32(out, kNativeHeaderSize);
  out.insert(out.end(), bytecode, bytecode + bytecode_size);

  js_free(context, bytecode);
  JS_FreeContext(context);
  JS_FreeRuntime(runtime);
  return out;
#endif
}


std::vector<unsigned char> compile_native_quickjs_module_bundle(const std::string& entry_source_name,
                                                               const std::vector<ModuleSourceRecord>& module_sources,
                                                               bool redact_source_metadata) {
  if (module_sources.empty()) throw std::runtime_error("QuickJS module bundle requires at least one module");
  if (module_sources.size() > 512u) throw std::runtime_error("QuickJS module bundle exceeds 512 modules");

  struct PackedModule {
    std::string name;
    std::vector<unsigned char> record;
  };
  std::vector<PackedModule> modules;
  modules.reserve(module_sources.size());
  std::uint32_t entry_index = 0xffffffffu;
  for (std::size_t i = 0; i < module_sources.size(); ++i) {
    const auto& source = module_sources[i];
    if (source.compile_name.empty()) throw std::runtime_error("QuickJS module bundle contains an empty module name");
    if (source.compile_name == entry_source_name || source.source_name == entry_source_name) {
      entry_index = static_cast<std::uint32_t>(i);
    }
    modules.push_back(PackedModule{
        source.compile_name,
        compile_native_quickjs_bytecode(source.source,
                                        source.compile_name,
                                        true,
                                        redact_source_metadata,
                                        &module_sources)});
  }
  if (entry_index == 0xffffffffu) throw std::runtime_error("QuickJS module bundle entry was not found: " + entry_source_name);

  constexpr std::uint32_t header_size = 24u;
  constexpr std::uint32_t entry_size = 16u;
  const std::uint64_t table_end64 = static_cast<std::uint64_t>(header_size) +
                                    static_cast<std::uint64_t>(modules.size()) * entry_size;
  if (table_end64 > 0xffffffffu) throw std::runtime_error("QuickJS module bundle table exceeds u32 limits");
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
      throw std::runtime_error("QuickJS module bundle entry exceeds u32 limits");
    }
    const std::uint32_t name_size = static_cast<std::uint32_t>(module.name.size());
    const std::uint32_t record_size = static_cast<std::uint32_t>(module.record.size());
    const std::uint64_t record_offset64 = static_cast<std::uint64_t>(cursor) + name_size;
    const std::uint64_t next64 = record_offset64 + record_size;
    if (next64 > 0xffffffffu) throw std::runtime_error("QuickJS module bundle exceeds u32 limits");
    table.push_back(TableEntry{cursor, name_size, static_cast<std::uint32_t>(record_offset64), record_size});
    cursor = static_cast<std::uint32_t>(next64);
  }

  std::vector<unsigned char> out;
  out.reserve(cursor);
  out.insert(out.end(), {'V','Q','J','S','M','B','0','4'});
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
  if (out.size() != cursor) throw std::runtime_error("QuickJS module bundle size accounting failed");
  return out;
}

bool is_protected_portable_bytecode(const std::vector<unsigned char>& bytes) {
  const char legacy_magic[] = {'V','Q','J','S','B','C','0','2'};
  return bytes.size() >= sizeof(legacy_magic) &&
         std::equal(bytes.begin(), bytes.begin() + sizeof(legacy_magic), legacy_magic);
}

bool is_native_quickjs_bytecode(const std::vector<unsigned char>& bytes) {
  return bytes.size() >= sizeof(kNativeMagic) &&
         std::equal(bytes.begin(), bytes.begin() + sizeof(kNativeMagic), std::begin(kNativeMagic));
}

std::string bytecode_format_name(BytecodeFormat format) {
  switch (format) {
    case BytecodeFormat::SourcePreservingV1: return "source-preserving-byte-buffer-record";
    case BytecodeFormat::ProtectedPortableV2: return "legacy-source-masked-record-rejected";
    case BytecodeFormat::NativeQuickJsObjectV1: return "native-quickjs-object-bytecode-v1";
    case BytecodeFormat::NativeQuickJsObjectV3: return "native-quickjs-object-bytecode-v3";
  }
  return "unknown";
}

std::string bytecode_manifest_text(const std::vector<BytecodeChunkRecord>& chunks,
                                   std::uint32_t runtime_abi,
                                   std::uint32_t package_version) {
  std::ostringstream out;
  out << "VENOM_QJS_BYTECODE_MANIFEST_V3\n"
      << "version=3\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "format=native-quickjs-object-bytecode-v3\n"
      << "magic=VQJSBC03\n"
      << "writer=JS_WriteObject\n"
      << "reader=JS_ReadObject\n"
      << "write_flags=BYTECODE|STRIP_SOURCE|STRIP_DEBUG\n"
      << "exec_claim=wasm-quickjs-reads-and-evaluates-object-bytecode\n"
      << "cache_policy=route-scoped-bytecode-hash\n"
      << "source_preserving=false\n"
      << "legacy_vqjsbc01_allowed=false\n"
      << "legacy_vqjsbc02_allowed=false\n"
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
  out << "VENOM_QJS_MODULE_RESOLVER_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "enabled=true\n"
      << "resolver_abi=1\n"
      << "mode=package-relative-module-map\n"
      << "resolve_export=venom_qjs_module_resolve\n"
      << "module_record_ptr=venom_qjs_module_record_ptr\n"
      << "module_record_size=venom_qjs_module_record_size\n"
      << "deny_remote_modules=true\n"
      << "literal_dynamic_import_bundle=VQJSMB04\n"
      << "chunk_count=" << chunk_count << "\n";
  return out.str();
}

std::string exception_abi_metadata_text(std::uint32_t runtime_abi,
                                        std::uint32_t package_version) {
  std::ostringstream out;
  out << "VENOM_QJS_EXCEPTION_ABI_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "enabled=true\n"
      << "exception_abi=1\n"
      << "exception_ptr=venom_qjs_exception_ptr\n"
      << "exception_size=venom_qjs_exception_size\n"
      << "exception_code=venom_qjs_exception_code\n"
      << "exception_clear=venom_qjs_exception_clear\n"
      << "schema=ok|abi|context|code|kind|message|sourceBytes|sourceHash\n";
  return out.str();
}

std::string host_trap_policy_metadata_text(bool strict_release,
                                           std::uint32_t runtime_abi,
                                           std::uint32_t package_version) {
  std::ostringstream out;
  out << "VENOM_QJS_HOST_TRAP_POLICY_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "enabled=true\n"
      << "policy=" << (strict_release ? "deny-unknown-host-imports" : "record-unknown-host-imports") << "\n"
      << "unknown_import=" << (strict_release ? "trap" : "record-and-fallback") << "\n"
      << "host_trap_policy_ptr=venom_qjs_host_trap_policy_ptr\n"
      << "host_trap_policy_size=venom_qjs_host_trap_policy_size\n"
      << "host_trap_policy_hash=venom_qjs_host_trap_policy_hash\n"
      << "known_import_table=quickjs-host-imports.vqhi\n";
  return out.str();
}

} // namespace venom::quickjs

#pragma once

#include <cstdint>
#include <string>

namespace venom::compiler {

enum JsChunkFlags : std::uint32_t {
  JsChunkInline = 1u << 0u,
  JsChunkModule = 1u << 1u,
  JsChunkDefer = 1u << 2u,
  JsChunkAsync = 1u << 3u,
  JsChunkExternalFile = 1u << 4u,
  JsChunkRemoteUrl = 1u << 5u,
  JsChunkBytecodeEncoded = 1u << 6u,
  JsChunkVendoredRemote = 1u << 7u,
  JsChunkDependency = 1u << 8u,
  JsChunkDynamicImport = 1u << 9u,
  JsChunkModulePreload = 1u << 10u,
  JsChunkBrowser = 1u << 11u,
  JsChunkProtectedModule = 1u << 12u,
};

struct JsChunk {
  std::string route;
  std::string source;
  std::string code;
  std::uint32_t order = 0;
  std::uint32_t flags = 0;
  std::uint32_t kind = 0;
  std::string execution_reason;
  std::uint32_t execution_confidence = 0;
  std::uint32_t ast_node_count = 0;
  std::uint32_t ast_function_count = 0;
  std::uint32_t ast_declaration_count = 0;
  std::uint32_t ast_import_count = 0;
  std::uint32_t ast_export_count = 0;
  std::uint32_t ast_top_level_declaration_count = 0;
  std::uint32_t ast_lexical_scope_count = 0;
  std::uint32_t ast_global_reference_count = 0;
  bool typescript_transpiled = false;
  std::uint32_t typescript_erased_declarations = 0;
  std::uint32_t typescript_erased_annotations = 0;
  std::uint32_t typescript_erased_assertions = 0;
};

struct ModuleGraphEdge {
  std::string route;
  std::string referrer;
  std::string specifier;
  std::string resolved;
  bool dynamic = false;
  bool preload = false;
};

} // namespace venom::compiler

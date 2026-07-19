#pragma once

#include "venom/pipeline/js.hpp"
#include "venom/internal/pipeline/js_planning.hpp"

#include <string>
#include <vector>

namespace venom::compiler::detail {

struct BridgeRewriteRecord {
  std::string source;
  std::string function;
  std::string candidate;
  std::string status;
  std::string reason;
  std::vector<std::string> lifted_dependencies;
  std::string input_contract_json;
  std::string output_contract_json;
  std::string input_typescript;
  std::string output_typescript;
};

struct RegistrySourceChunk {
  std::string id;
  std::string source;
  std::vector<std::string> candidates;
};

struct BridgeRewriteResult {
  std::string registry_source;
  std::vector<RegistrySourceChunk> registry_chunks;
  std::string typescript;
  std::vector<BridgeRewriteRecord> records;
};

struct ProtectedModuleRewriteResult {
  std::string registry_source;
  std::vector<RegistrySourceChunk> registry_chunks;
  std::string typescript;
  std::vector<BridgeRewriteRecord> records;
  std::vector<std::pair<std::string, std::string>> lowered_browser_imports;
};

ProtectedModuleRewriteResult apply_protected_module_rewrites(
    std::vector<JsChunk>& chunks,
    const std::string& bridge_id_salt,
    bool development);

BridgeRewriteResult apply_bridge_rewrites(
    std::vector<JsChunk>& chunks,
    const std::vector<FunctionExtractionRecord>& extraction_records,
    const std::string& bridge_id_salt);

std::string make_bridge_rewrite_report_json(
    const std::vector<BridgeRewriteRecord>& records);

std::string make_protected_contracts_json(
    const std::vector<BridgeRewriteRecord>& records);

}  // namespace venom::compiler::detail

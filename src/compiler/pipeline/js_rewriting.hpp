#pragma once

#include "compiler/pipeline/js.hpp"
#include "compiler/pipeline/js_planning.hpp"

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
};

struct BridgeRewriteResult {
  std::string registry_source;
  std::vector<BridgeRewriteRecord> records;
};

struct ProtectedModuleRewriteResult {
  std::string registry_source;
  std::string typescript;
  std::vector<BridgeRewriteRecord> records;
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

}  // namespace venom::compiler::detail

#pragma once

#include "compiler/pipeline/js.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace venom::compiler::detail {

struct FunctionRealmRecord {
  std::string route;
  std::string source;
  std::string name;
  std::string execution;
  std::string reason;
  std::uint32_t line = 0;
  bool promoted_whole_file = false;
  bool isolated = false;
};

struct FunctionExtractionRecord {
  std::string route;
  std::string source;
  std::string name;
  std::string requested_realm;
  std::string disposition;
  std::string bridge_mode;
  std::string reason;
  std::vector<std::string> blockers;
  std::uint32_t line = 0;
  bool isolated = false;
};

std::vector<FunctionRealmRecord> scan_function_realm_directives(const JsChunk& chunk);
std::vector<FunctionRealmRecord> apply_function_realm_planning(std::vector<JsChunk>& chunks);
std::vector<FunctionRealmRecord> collect_function_realm_records(const std::vector<JsChunk>& chunks);
std::vector<FunctionExtractionRecord> analyze_function_extraction(
    const std::vector<JsChunk>& chunks,
    const std::vector<FunctionRealmRecord>& functions);

std::string make_function_plan_text(const std::vector<FunctionRealmRecord>& records);
std::string make_function_plan_json(const std::vector<FunctionRealmRecord>& records);
std::string make_extraction_plan_text(const std::vector<FunctionExtractionRecord>& records);
std::string make_extraction_plan_json(const std::vector<FunctionExtractionRecord>& records);
std::string make_realm_bridge_contract_json(const std::vector<FunctionExtractionRecord>& records);

} // namespace venom::compiler::detail

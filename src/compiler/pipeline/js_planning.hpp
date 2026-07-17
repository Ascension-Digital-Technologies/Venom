#pragma once

#include "compiler/pipeline/js.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace venom::compiler::detail {

struct FunctionRuntimeRecord {
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
  std::string requested_runtime;
  std::string disposition;
  std::string bridge_mode;
  std::string reason;
  std::vector<std::string> blockers;
  std::uint32_t line = 0;
  bool isolated = false;
};

std::vector<FunctionRuntimeRecord> scan_function_runtime_directives(const JsChunk& chunk);
std::vector<FunctionRuntimeRecord> apply_function_runtime_planning(std::vector<JsChunk>& chunks);
std::vector<FunctionRuntimeRecord> collect_function_runtime_records(const std::vector<JsChunk>& chunks);
std::vector<FunctionExtractionRecord> analyze_function_extraction(
    const std::vector<JsChunk>& chunks,
    const std::vector<FunctionRuntimeRecord>& functions);

std::string make_function_plan_text(const std::vector<FunctionRuntimeRecord>& records);
std::string make_function_plan_json(const std::vector<FunctionRuntimeRecord>& records);
std::string make_extraction_plan_text(const std::vector<FunctionExtractionRecord>& records);
std::string make_extraction_plan_json(const std::vector<FunctionExtractionRecord>& records);
std::string make_runtime_bridge_contract_json(const std::vector<FunctionExtractionRecord>& records);

} // namespace venom::compiler::detail

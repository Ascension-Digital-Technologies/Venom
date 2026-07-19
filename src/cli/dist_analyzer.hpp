#pragma once

#include "venom/core/options.hpp"
#include <filesystem>

namespace venom::compiler {

struct DistAnalyzeOptions {
  std::filesystem::path input;
  OutputFormat format = OutputFormat::Text;
};

bool analyze_distribution(const DistAnalyzeOptions& options);

} // namespace venom::compiler

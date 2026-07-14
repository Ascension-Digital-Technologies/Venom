#pragma once
#include "compiler/commands/cli.hpp"
namespace venom::compiler {
bool run_compatibility_check(const CompatibilityOptions& options);
void run_capability_analysis(const CompatibilityOptions& options);
void print_contracts(OutputFormat format);
}

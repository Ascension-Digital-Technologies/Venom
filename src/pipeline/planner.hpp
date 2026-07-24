#pragma once
#include "core/options.hpp"
namespace venom::compiler {
bool run_protection_plan(const PlannerOptions& options);
bool enforce_build_protection_plan(const BuildOptions& options);
}

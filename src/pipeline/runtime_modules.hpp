#pragma once

#include "capability_analysis.hpp"
#include "venom/core/site.hpp"
#include "venom/core/options.hpp"

#include <string>
#include <vector>

namespace venom::compiler {

struct RuntimeModulePlan {
  bool network = false;
  bool timers = false;
  bool events = false;
  bool storage = false;
  bool crypto = false;
  bool navigation = false;
  bool forms = false;
  bool observers = false;
  bool animation = false;

  std::vector<std::string> enabled_modules() const;
};

RuntimeModulePlan plan_runtime_modules(const SiteGraph& graph, const CapabilityGraph& capabilities, const BuildOptions& options);
std::string specialize_runtime_modules(std::string source, const RuntimeModulePlan& plan);
std::string runtime_module_plan_json(const RuntimeModulePlan& plan);

} // namespace venom::compiler

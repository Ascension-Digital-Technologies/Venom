#pragma once

#include <string>
#include <vector>

namespace venom::compiler {

struct LiftedFunctionDependency {
  std::string name;
  std::string declaration;
};

struct FunctionDependencyResolution {
  bool success = false;
  std::string reason;
  std::vector<LiftedFunctionDependency> dependencies;
};

FunctionDependencyResolution resolve_liftable_function_dependencies(
    const std::string& source,
    const std::string& target_declaration,
    const std::string& target_name,
    const std::string& target_params);

} // namespace venom::compiler

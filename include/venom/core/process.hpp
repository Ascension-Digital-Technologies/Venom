#pragma once

#include <string>
#include <vector>

namespace venom::compiler {

// Runs a child process without invoking a shell. Arguments must not include the
// executable itself. Returns the process exit code, or a negative value when
// the process could not be started or waited for.
int run_process(const std::string& executable, const std::vector<std::string>& arguments);

} // namespace venom::compiler

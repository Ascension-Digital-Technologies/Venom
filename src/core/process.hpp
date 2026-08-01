#pragma once

#include <string>
#include <vector>

namespace venom::compiler {

// Runs a child process without invoking a shell. Arguments must not include the
// executable itself. Returns the process exit code, or a negative value when
// the process could not be started or waited for (including when it was
// killed after exceeding timeout_ms). timeout_ms <= 0 means wait indefinitely,
// matching the historical behavior.
int run_process(const std::string& executable, const std::vector<std::string>& arguments,
                int timeout_ms = 0);

} // namespace venom::compiler

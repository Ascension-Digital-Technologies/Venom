#pragma once

#include "core/options.hpp"

namespace venom::compiler {

bool generate_key_file(const KeygenOptions& options);
bool release_check(const ReleaseCheckOptions& options);
void load_package_key_file_for_process(const std::filesystem::path& key_file);

} // namespace venom::compiler

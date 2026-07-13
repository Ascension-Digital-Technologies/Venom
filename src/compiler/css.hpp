#pragma once

#include "compiler/assets.hpp"
#include "compiler/site.hpp"

#include <string>

namespace venom::compiler {

std::string bundle_css(const SiteGraph& graph, const AssetPipeline& assets);

} // namespace venom::compiler

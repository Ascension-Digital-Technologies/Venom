#pragma once

#include "assets.hpp"
#include "venom/core/site.hpp"

#include <string>

namespace venom::compiler {

std::string bundle_css(const SiteGraph& graph, const AssetPipeline& assets);

} // namespace venom::compiler

#pragma once

#include "pipeline/assets.hpp"
#include "core/site.hpp"
#include "pipeline/js.hpp"

#include <string>

namespace venom::compiler {

std::string bundle_css(const SiteGraph& graph, const AssetPipeline& assets, const std::vector<JsChunk>& scripts);

} // namespace venom::compiler

#pragma once

#include "compiler/pipeline/js.hpp"

namespace venom::compiler::detail {

std::vector<JsChunk> collect_script_chunks(const SiteGraph& graph,
                                           const RemoteVendorOptions& remote_options,
                                           std::vector<RemoteVendorRecord>& remote_vendors,
                                           std::vector<ModuleGraphEdge>& module_edges);
void close_classic_browser_runtimes(std::vector<JsChunk>& chunks);
bool has_module_references(const std::string& source);

} // namespace venom::compiler::detail

#pragma once

#include "venom/remote/remote.hpp"
#include "venom/core/profile.hpp"
#include "venom/core/site.hpp"
#include "planning/package_plan.hpp"
#include "assets.hpp"
#include "venom/pipeline/build.hpp"
#include "build_package_metadata.hpp"
#include "html.hpp"
#include "venom/pipeline/js.hpp"
#include "venom/vm/polymorph.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace venom::compiler::package_sections {

struct Inputs {
  const BuildOptions& options;
  const Profile& profile;
  const SiteGraph& graph;
  const CompiledHtmlRoutes& compiled_routes;
  const JsBridge& js_bridge;
  const RemoteVendorOptions& remote_vendor_options;
  const std::filesystem::path& vendor_lock_path;
  const std::string& vendor_lock_digest;
  const std::string& vendor_lock_mode;
  const build_package_detail::ReleaseBuildPolicy& release_policy;
  const AssetPipeline& assets;
  const venom::vm::PolymorphicPlan& polymorphic_plan;
  const std::string& runtime_name;
  const std::string& runtime_source;
  const std::string& wasm_name;
  const std::vector<unsigned char>& wasm_bytes;
  const std::string& style_name;
  const std::string& css;
  const std::string& quickjs_engine_name;
  const std::string& quickjs_engine_source;
  const std::string& quickjs_wasm_name;
  const std::vector<unsigned char>& quickjs_wasm_bytes;
  const std::string& worker_name;
  const std::string& worker_source;
  const std::string& quickjs_probe;
  const std::string& js_preview;
  bool wasm_runtime = false;
  bool hardened_release_asset = false;
  bool production_hardening = false;
  bool external_lazy_registry = false;
};

struct Result {
  package_plan::Plan plan;
  std::string package_binding_token;
  std::uint32_t package_flags = 0u;
  std::uint32_t layout_max_padding = 0u;
};

std::vector<JsChunk> select_lazy_route_scripts(const std::vector<JsChunk>& chunks, const std::string& route);

Result make_plan(const Inputs& inputs);

} // namespace venom::compiler::package_sections

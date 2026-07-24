#pragma once

#include "pipeline/build.hpp"
#include "pipeline/planning/package_plan.hpp"
#include "pipeline/js.hpp"
#include "core/profile.hpp"
#include "package/format.hpp"
#include "package/writer.hpp"
#include "vm/encoder.hpp"
#include "vm/polymorph.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace venom::compiler::build_package_detail {

using PendingPackageSection = venom::compiler::package_plan::Section;

struct ReleaseBuildPolicy {
  bool release_like = false;
  bool has_scripts = false;
  bool backend_is_scaffold = true;
  bool backend_is_real_browser = false;
  bool fallback_allowed = false;
  bool fallback_denied = false;
  bool unsafe_release_fallback = false;
  std::string backend;
  std::string decision;
  std::string reason;
};

ReleaseBuildPolicy resolve_release_build_policy(const Profile&, const BuildOptions&, std::size_t);
void enforce_release_build_policy(const ReleaseBuildPolicy&, std::size_t);
std::string make_release_build_policy_metadata(const Profile&, const BuildOptions&, const ReleaseBuildPolicy&, std::size_t);
std::string selected_section_sealer_name(const Profile&, const std::string&);
venom::package::SectionCryptoProvider selected_writer_crypto_provider(const std::string&);
std::string stored_section_name_for_metadata(const Profile&, venom::package::SectionType, const std::string&);
std::string make_package_binding_token(bool, const std::string&);
std::string package_section_order_digest(const Profile&, const std::vector<PendingPackageSection>&);
std::string make_package_layout_metadata(const Profile&, const BuildOptions&, const venom::vm::PolymorphicPlan&, const std::vector<PendingPackageSection>&, std::uint32_t);
void append_u32_local(std::vector<unsigned char>&, std::uint32_t);
std::vector<unsigned char> make_turbojs_abi_fingerprint();
std::vector<unsigned char> make_release_diversification_table(const venom::vm::PolymorphicPlan&);
std::vector<unsigned char> make_single_route_bytecode_section(const venom::vm::Program&, const venom::vm::PolymorphicPlan&);
std::vector<unsigned char> encode_route_script_bundle(const std::vector<JsChunk>&, const graph::CanonicalModuleGraph&, bool);
std::string lazy_route_section_name(const std::string&);
std::string lazy_script_section_name(const std::string&);
struct LazySectionPlanRow {
  std::string route;
  std::string route_section;
  std::uint32_t instruction_count = 0;
  std::uint32_t route_bytecode_size = 0;
  std::string script_section;
  std::uint32_t script_count = 0;
};

std::string make_lazy_sections_metadata(const Profile&, const BuildOptions&, const std::vector<LazySectionPlanRow>&);
std::string release_threat_model_id(const BuildOptions&);
std::string release_secret_model(const BuildOptions&);
std::string make_release_profile_metadata(const Profile&, const BuildOptions&, const ReleaseBuildPolicy&, std::size_t);
std::string make_package_binding_metadata(const Profile&, const BuildOptions&, const std::string&, const std::string&, const std::string&, const std::string&, const std::vector<unsigned char>&, const std::string&, const std::string&, const std::string&, const std::string&, const std::string&, const std::vector<unsigned char>&, const std::string&, const std::string&);
std::string make_integrity_metadata(const Profile&, const std::string&, const std::vector<PendingPackageSection>&, const std::string&);
bool package_feature_required_in_release(const PendingPackageSection&);
std::string make_package_features_metadata(const Profile&, const std::string&, const std::vector<PendingPackageSection>&, const std::string&);

} // namespace venom::compiler::build_package_detail

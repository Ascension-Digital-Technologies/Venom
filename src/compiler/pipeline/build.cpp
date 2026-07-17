#include "compiler/services/runtime_modules.hpp"
#include "compiler/pipeline/build.hpp"
#include "compiler/pipeline/build_support.hpp"
#include "compiler/pipeline/build_package_metadata.hpp"
#include "compiler/pipeline/build_runtime_metadata.hpp"
#include "compiler/core/version.hpp"
#include "compiler/core/planner.hpp"

#include "compiler/pipeline/assets.hpp"
#include "compiler/pipeline/capability_analysis.hpp"
#include "compiler/pipeline/css.hpp"
#include "compiler/pipeline/html.hpp"
#include "compiler/pipeline/js.hpp"
#include "compiler/core/profile.hpp"
#include "compiler/core/process.hpp"
#include "compiler/pipeline/security.hpp"
#include "compiler/core/site.hpp"
#include "package/hash.hpp"
#include "package/crypto.hpp"
#include "package/reader.hpp"
#include "package/writer.hpp"
#include "quickjs/abi.hpp"
#include "quickjs/bytecode.hpp"
#include "quickjs/bridge.hpp"
#include "quickjs/engine.hpp"
#include "vm/polymorph.hpp"
#include "vm/encoder.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>

namespace venom::compiler {

namespace {
using namespace build_detail;
using namespace build_package_detail;
using namespace build_runtime_detail;

class BuildTrace {
 public:
  explicit BuildTrace(const BuildOptions& options)
      : enabled_(options.format != OutputFormat::Json && options.verbosity > 0),
        detailed_(options.format != OutputFormat::Json && options.verbosity > 1),
        started_(Clock::now()), last_(started_) {}

  void banner(const BuildOptions& options) const {
    if (!enabled_) return;
    std::cout << "\nVENOM  Protected build pipeline\n"
              << "       native compiler + QuickJS/WASM runtime\n"
              << "------------------------------------------------------------------------\n"
              << "[INFO]     Input:      " << std::filesystem::absolute(options.input).string() << "\n"
              << "[INFO]     Output:     " << std::filesystem::absolute(options.output).string() << "\n"
              << "[INFO]     Profile:    " << options.profile << "\n"
              << "[INFO]     Protection: " << options.protection_level << "\n"
              << "[INFO]     Runtime:    " << options.runtime << " / " << options.quickjs_backend << "\n";
  }

  void step(const std::string& message) {
    if (!enabled_) return;
    const auto now = Clock::now();
    const auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_).count();
    std::cout << "[STEP]     " << message;
    if (last_ != started_) std::cout << "  (previous " << format_duration(delta) << ")";
    std::cout << "\n";
    last_ = now;
  }

  void info(const std::string& message) const {
    if (enabled_) std::cout << "[INFO]     " << message << "\n";
  }

  void detail(const std::string& message) const {
    if (detailed_) std::cout << "[DETAIL]   " << message << "\n";
  }

  void success(const std::string& message) const {
    if (!enabled_) return;
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started_).count();
    std::cout << "[SUCCESS]  " << message << " (" << format_duration(elapsed) << ")\n";
  }

 private:
  using Clock = std::chrono::steady_clock;
  static std::string format_duration(long long milliseconds) {
    std::ostringstream out;
    if (milliseconds >= 1000) {
      out << std::fixed << std::setprecision(2) << (static_cast<double>(milliseconds) / 1000.0) << "s";
    } else {
      out << milliseconds << "ms";
    }
    return out.str();
  }

  bool enabled_;
  bool detailed_;
  Clock::time_point started_;
  Clock::time_point last_;
};

bool should_encrypt_section(const Profile& profile, venom::package::SectionType type, const std::string& name) {
  if (!profile.crypto_provider_ready || profile.kind == ProfileKind::Dev) {
    return false;
  }
  if (type == venom::package::SectionType::Asset) {
    return false;
  }
  if (type == venom::package::SectionType::Css) {
    return false;
  }
  (void)name;
  return true;
}


std::size_t count_marker(const std::vector<unsigned char>& bytes, const std::string& marker) {
  if (marker.empty() || bytes.size() < marker.size()) {
    return 0;
  }
  std::size_t count = 0;
  auto it = bytes.begin();
  while ((it = std::search(it, bytes.end(), marker.begin(), marker.end())) != bytes.end()) {
    ++count;
    ++it;
  }
  return count;
}

void print_build_protection_report(const Profile& profile,
                                   const BuildOptions& options,
                                   const std::filesystem::path& package_path,
                                   const std::vector<unsigned char>& package_bytes,
                                   const venom::package::Package& package,
                                   bool external_manifest) {
  std::size_t encrypted_sections = 0;
  std::size_t compressed_sections = 0;
  bool manifest_section = false;
  bool package_binding = false;
  bool release_profile = false;
  bool layout_polymorphic = false;
  bool lazy_sections = false;
  std::size_t quickjs_bytecode_records = 0;
  for (const auto& section : package.sections) {
    if ((section.flags & venom::package::SectionFlagEncrypted) != 0u) {
      ++encrypted_sections;
    }
    if ((section.flags & venom::package::SectionFlagCompressed) != 0u) {
      ++compressed_sections;
    }
    if (section.type == venom::package::SectionType::Manifest && section.name == "manifest.txt") {
      manifest_section = true;
    }
    if (section.type == venom::package::SectionType::Integrity && venom::package::section_name_matches(section.type, section.name, "package-binding.vbind")) {
      package_binding = true;
    }
    if (section.type == venom::package::SectionType::Integrity && venom::package::section_name_matches(section.type, section.name, "release-profile.vrpf")) {
      release_profile = true;
    }
    if (section.type == venom::package::SectionType::Integrity && venom::package::section_name_matches(section.type, section.name, "package-layout.vlay")) {
      layout_polymorphic = true;
    }
    if (section.type == venom::package::SectionType::Integrity && venom::package::section_name_matches(section.type, section.name, "lazy-sections.vlazy")) {
      lazy_sections = true;
    }
    if (section.type == venom::package::SectionType::JavaScript) {
      quickjs_bytecode_records += count_marker(section.data, "VQJSE006");
    }
  }
  const auto runtime_sections = count_marker(package_bytes, "VAEAD001");
  const auto sodium_sections = count_marker(package_bytes, "VSODIUM1");
  const auto legacy_sections = count_marker(package_bytes, "VSEAL001");
  const bool audited = sodium_sections != 0u;
  const bool browser_runnable = sodium_sections == 0u;
  const bool release_pass = profile.kind != ProfileKind::Dev &&
                            encrypted_sections != 0u &&
                            legacy_sections == 0u &&
                            !external_manifest &&
                            !manifest_section &&
                            package_binding &&
                            release_profile &&
                            (!options.require_audited_crypto || audited);
  std::cout << "Protection report:\n"
            << "  profile: " << profile.name << "\n"
            << "  target: " << options.security_target << "\n"
            << "  package: " << package_path.string() << "\n"
            << "  protected_sections: " << encrypted_sections << "\n"
            << "  compressed_sections: " << compressed_sections << "\n"
            << "  provider: " << selected_section_sealer_name(profile, options.crypto_provider) << "\n"
            << "  provider_runtime_sections: " << runtime_sections << "\n"
            << "  provider_libsodium_sections: " << sodium_sections << "\n"
            << "  provider_legacy_sections: " << legacy_sections << "\n"
            << "  quickjs_bytecode_records: " << quickjs_bytecode_records << "\n"
            << "  package_binding: " << (package_binding ? "yes" : "no") << "\n"
            << "  release_profile: " << (release_profile ? "yes" : "no") << "\n"
            << "  layout_polymorphic: " << (layout_polymorphic ? "yes" : "no") << "\n"
            << "  lazy_sections: " << (lazy_sections ? "yes" : "no") << "\n"
            << "  debug_metadata: " << (profile.strip_debug_metadata ? "stripped" : "present") << "\n"
            << "  external_debug_manifest: " << (external_manifest ? "yes" : "no") << "\n"
            << "  package_manifest_section: " << (manifest_section ? "yes" : "no") << "\n"
            << "  browser_executable: " << (browser_runnable ? "yes" : "no") << "\n"
            << "  native_private_crypto: " << (audited ? "yes" : "no") << "\n"
            << "  release_status: " << (release_pass ? "PASS" : "CHECK") << "\n";
}

bool should_ship_manifest_section(const Profile& profile, const BuildOptions& options) {
  return !profile.strip_debug_metadata || options.emit_debug_manifest;
}

bool should_emit_external_asset_manifest(const Profile& profile, const BuildOptions& options) {
  return profile.name == "dev" || options.emit_debug_manifest;
}

void shuffle_package_sections(std::vector<PendingPackageSection>& sections, const venom::vm::PolymorphicPlan& poly) {
  if (!poly.enabled || sections.size() < 3) {
    return;
  }

  venom::vm::DiversificationRng rng(poly, "package-section-order");
  std::shuffle(sections.begin(), sections.end(), rng);
}
} // namespace

bool build_site(const BuildOptions& requested_options) {
  const BuildOptions& options = requested_options;
  BuildTrace trace(options);
  trace.banner(options);
  trace.step("Validate build policy and load configuration");
  enforce_build_protection_plan(options);
  if (!options.key_file.empty()) {
    load_package_key_file_for_process(options.key_file);
  }
  if (options.require_audited_crypto && options.crypto_provider != "libsodium") {
    throw std::runtime_error("--require-audited-crypto is not available for browser dev/prod builds");
  }
  const auto profile = resolve_profile(options.profile);
  trace.detail("profile flags: fail_closed=" + std::string(profile.fail_closed ? "yes" : "no") +
               " polymorphic=" + std::string(profile.polymorphic ? "yes" : "no") +
               " strip_debug=" + std::string(profile.strip_debug_metadata ? "yes" : "no"));
  trace.step("Verify embedded QuickJS runtime and discover site graph");
  require_real_embedded_quickjs_runtime();
  const auto graph = collect_site(options.input);
  trace.info("Discovered " + std::to_string(graph.files.size()) + " files across " + std::to_string(graph.routes.size()) + " HTML routes");

  if (graph.routes.empty()) {
    throw std::runtime_error("site has no html routes");
  }

  const auto vendor_lock_path = options.vendor_lock.empty()
      ? (graph.root / "venom.lock")
      : std::filesystem::absolute(options.vendor_lock);
  const auto vendor_lock = load_remote_vendor_lock(vendor_lock_path);

  RemoteVendorOptions remote_vendor_options;
  remote_vendor_options.enabled = true;
  remote_vendor_options.offline = options.vendor_offline;
  remote_vendor_options.refresh = options.refresh_vendors;
  remote_vendor_options.cache_directory = options.vendor_cache.empty()
      ? (std::filesystem::absolute(options.output).parent_path() / ".venom-cache" / graph.root.filename() / "remote")
      : std::filesystem::absolute(options.vendor_cache);
  remote_vendor_options.lock_file = vendor_lock_path;
  remote_vendor_options.lock_present = vendor_lock.present;
  remote_vendor_options.lock_entries = vendor_lock.entries;
  {
    const char* reproducible_epoch = std::getenv("SOURCE_DATE_EPOCH");
    if (reproducible_epoch && *reproducible_epoch) {
      remote_vendor_options.bridge_id_salt = std::string{"epoch:"} + reproducible_epoch;
    } else {
      const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
      remote_vendor_options.bridge_id_salt = std::string{"nonce:"} + std::to_string(now);
    }
  }
  trace.step("Analyze JavaScript realms, modules, annotations, and dependencies");
  const auto js_bridge = compile_js_bridge(graph, remote_vendor_options, profile.name == "dev");
  trace.info("Planned " + std::to_string(js_bridge.chunks.size()) + " protected script chunks and " +
             std::to_string(js_bridge.protected_exports.size()) + " protected exports");
  trace.detail("module graph: " + std::to_string(js_bridge.module_edges.size()) + " edges; remote vendors: " +
               std::to_string(js_bridge.remote_vendors.size()));
  require_embedded_quickjs_module_bundle_runtime(js_bridge);
  if (options.vendor_offline && !vendor_lock.present && !js_bridge.remote_vendors.empty()) {
    throw std::runtime_error("offline remote vendoring requires venom.lock; run one reviewed online build first");
  }
  validate_remote_vendor_lock_coverage(vendor_lock, js_bridge.remote_vendors, options.refresh_vendors);
  trace.step("Compile and validate protected QuickJS bytecode records");
  // Validate every protected script/bytecode record before touching the output
  // directory. A capacity failure must never leave a partial production dist.
  (void)make_quickjs_bytecode_records(js_bridge, profile.kind == ProfileKind::Prod || profile.strip_debug_metadata);
  const auto vendor_lock_digest = remote_vendor_lock_sha256(js_bridge.remote_vendors);
  const std::string vendor_lock_mode = options.refresh_vendors
      ? "refreshed"
      : (vendor_lock.present ? "enforced" : "generated");
  const auto release_policy = resolve_release_build_policy(profile, options, js_bridge.chunks.size());
  enforce_release_build_policy(release_policy, js_bridge.chunks.size());

  const bool hardened_release_asset = profile.fail_closed || profile.kind == ProfileKind::Prod || options.strict_release;
  const bool production_hardening = profile.name == "prod";
  if (hardened_release_asset) {
    const auto bytecode_format = wasm_truth_value("bytecode_format", "");
    const auto native_reader = wasm_truth_value("native_object_reader", "");
    const auto source_materialization = wasm_truth_value("source_materialization", "");
    if (bytecode_format != "VQJSBC03" || native_reader != "JS_ReadObject" || source_materialization != "false") {
      throw std::runtime_error(
          "embedded QuickJS WASM is legacy or incompatible with native VQJSBC03 records; "
          "rebuild and embed it with scripts/build-quickjs-wasm before producing a protected release");
    }
  }

  trace.step("Prepare clean distribution layout");
  std::filesystem::remove_all(options.output);
  const auto assets_dir = options.output / "assets";
  std::filesystem::create_directories(assets_dir);

  trace.step("Process static assets and bundle stylesheets");
  const auto assets = build_asset_pipeline(graph, options.hashed_assets, production_hardening);
  const auto css = bundle_css(graph, assets);
  const bool wasm_runtime = options.runtime == "wasm";
  if (options.runtime != "js" && options.runtime != "wasm") {
    throw std::runtime_error("unsupported runtime mode: " + options.runtime);
  }
  const auto deterministic_seed = options.diversification_seed != 0u ? options.diversification_seed : (profile.deterministic ? 0xC0FFEEu : 0u);
  const auto poly = venom::vm::make_polymorphic_plan(deterministic_seed, profile.polymorphic);
  trace.info("Prepared " + std::to_string(assets.records.size()) + " public assets and " + std::to_string(css.size()) + " CSS bytes");
  trace.step("Plan runtime capabilities and build-specific polymorphism");
  const auto capability_graph = analyze_capabilities(graph.root);
  const auto runtime_modules = plan_runtime_modules(graph, capability_graph);
  const auto wasm_bytes = wasm_runtime ? make_runtime_wasm_module() : std::vector<unsigned char>{};
  const auto wasm_file_name = wasm_runtime ? named_output(hardened_release_asset ? "rw" : "runtime", ".wasm", wasm_bytes, options.hashed_assets) : std::string{};
  const auto wasm_name = hardened_release_asset && wasm_runtime ? "runtime/" + wasm_file_name : wasm_file_name;
  // The bridge and its companion WASM live beside each other in protected output,
  // so the generated module uses the basename while package metadata records the
  // canonical distribution path.
  const auto wasm_runtime_asset_ref = hardened_release_asset ? wasm_name : wasm_file_name;
  trace.detail("polymorphic seed: " + std::to_string(poly.seed));
  trace.step("Generate runtime bridge, route bytecode, worker, and engine assets");
  auto runtime = wasm_runtime ? make_runtime_wasm_bridge_js(graph, wasm_runtime_asset_ref, hardened_release_asset, &poly) : make_runtime_js(graph, hardened_release_asset);
  runtime = specialize_runtime_modules(std::move(runtime), runtime_modules);
  const auto js_preview = js_bridge.preview.empty() ? bundle_js_preview(graph) : js_bridge.preview;
  const auto compiled_routes = compile_html_routes(graph, poly);

  venom::quickjs::Engine engine;
  const auto qjs_probe = engine.eval_string("'quickjs:' + (1 + 1)");

  const auto style_file_name = named_output(hardened_release_asset ? "s" : "style", ".css", css, options.hashed_assets);
  const auto style_name = hardened_release_asset ? "style/" + style_file_name : style_file_name;
  auto quickjs_engine_module = make_quickjs_engine_module_js(hardened_release_asset);
  auto quickjs_wasm_bytes = make_quickjs_runtime_wasm_module();
  const auto bridge_invoke_opcode = bridge_protocol_opcode(remote_vendor_options.bridge_id_salt, "invoke");
  const auto bridge_cancel_opcode = bridge_protocol_opcode(remote_vendor_options.bridge_id_salt, "cancel");
  const auto bridge_result_opcode = bridge_protocol_opcode(remote_vendor_options.bridge_id_salt, "result");
  const auto bridge_error_opcode = bridge_protocol_opcode(remote_vendor_options.bridge_id_salt, "error");
  auto worker_runtime = make_worker_runtime_js(js_bridge.bridge_candidate_ids, js_bridge.bridge_registry_bytecode,
      bridge_invoke_opcode, bridge_cancel_opcode, bridge_result_opcode, bridge_error_opcode);
  if (production_hardening) {
    const auto wasm_export_aliases = compact_quickjs_wasm_exports(quickjs_wasm_bytes, remote_vendor_options.bridge_id_salt);
    redact_quickjs_wasm_abi_table(quickjs_wasm_bytes, remote_vendor_options.bridge_id_salt);
    apply_wasm_export_aliases(runtime, wasm_export_aliases);
    apply_wasm_export_aliases(quickjs_engine_module, wasm_export_aliases);
    apply_wasm_export_aliases(worker_runtime, wasm_export_aliases);
    redact_unmapped_quickjs_symbols(runtime, remote_vendor_options.bridge_id_salt);
    redact_unmapped_quickjs_symbols(quickjs_engine_module, remote_vendor_options.bridge_id_salt);
    redact_unmapped_quickjs_symbols(worker_runtime, remote_vendor_options.bridge_id_salt);
    runtime = harden_release_js_asset(std::move(runtime));
    runtime = ast_harden_release_js("runtime", runtime);
    validate_protected_js_asset("runtime", runtime);
    quickjs_engine_module = harden_release_js_asset(std::move(quickjs_engine_module));
    quickjs_engine_module = ast_harden_release_js("engine", quickjs_engine_module);
    validate_protected_js_asset("quickjs-engine", quickjs_engine_module);
  }
  if (!production_hardening) {
    // Development output remains readable and uses the descriptive ABI names.
  }
  const auto runtime_file_name = named_output((hardened_release_asset && wasm_runtime) ? "r" : (wasm_runtime ? "runtime-js-bridge" : "runtime"), ".js", runtime, options.hashed_assets);
  const auto runtime_name = hardened_release_asset ? "runtime/" + runtime_file_name : runtime_file_name;
  const auto quickjs_engine_file_name = named_output(hardened_release_asset ? "engine" : "quickjs-engine", ".js", quickjs_engine_module, options.hashed_assets);
  const auto quickjs_engine_name = hardened_release_asset ? "runtime/" + quickjs_engine_file_name : quickjs_engine_file_name;
  const auto quickjs_wasm_file_name = named_output(hardened_release_asset ? "runtime" : "quickjs-runtime", ".wasm", quickjs_wasm_bytes, options.hashed_assets);
  const auto quickjs_wasm_name = hardened_release_asset ? "runtime/" + quickjs_wasm_file_name : quickjs_wasm_file_name;
  if (production_hardening) {
    worker_runtime = harden_release_js_asset(std::move(worker_runtime));
    worker_runtime = ast_harden_release_js("worker", worker_runtime);
    validate_protected_js_asset("worker", worker_runtime);
  }
  const auto worker_file_name = hardened_release_asset ? named_output("worker", ".js", worker_runtime, options.hashed_assets) : std::string{};
  const auto worker_name = hardened_release_asset ? "workers/" + worker_file_name : std::string{};
  const auto package_binding_token = make_package_binding_token(options.diversification_seed != 0u || profile.deterministic, profile.name + "|" + options.runtime + "|" + runtime_name + "|" + wasm_name + "|" + quickjs_engine_name + "|" + quickjs_wasm_name + "|" + worker_name);
  const auto package_flags = profile.package_flags |
      ((options.strict_release || profile.fail_closed) ? venom::package::PackageFlagReleaseProfile : 0u) |
      venom::package::PackageFlagTimerBridge |
      venom::package::PackageFlagEventQueue |
      venom::package::PackageFlagQuickJsBridge |
      venom::package::PackageFlagScriptIsolation |
      venom::package::PackageFlagScriptPolicy |
      venom::package::PackageFlagQuickJsChunks |
      venom::package::PackageFlagQuickJsEngine |
      venom::package::PackageFlagScriptEngineFallback |
      venom::package::PackageFlagQuickJsEngineModule |
      venom::package::PackageFlagQuickJsContextLifecycle |
      venom::package::PackageFlagHostCapabilities |
      venom::package::PackageFlagQuickJsAdapterDiagnostics |
      venom::package::PackageFlagQuickJsWasmRuntime |
      venom::package::PackageFlagQuickJsSourceTransfer |
      venom::package::PackageFlagQuickJsConsoleBridge |
      venom::package::PackageFlagQuickJsExecutionRecords |
      venom::package::PackageFlagQuickJsResultBridge |
      venom::package::PackageFlagQuickJsFallbackPolicy |
      venom::package::PackageFlagQuickJsEngineBackend |
      (wasm_runtime ? (venom::package::PackageFlagWasmRuntime | venom::package::PackageFlagBinaryDomOps) : 0u);

  std::vector<PendingPackageSection> package_sections;
  const auto add_package_section = [&](venom::package::SectionType type,
                                       std::string name,
                                       std::vector<unsigned char> data,
                                       std::uint32_t flags = venom::package::SectionFlagNone) {
    if (should_encrypt_section(profile, type, name)) {
      flags |= venom::package::SectionFlagEncrypted;
    }
    package_sections.push_back({type, flags, std::move(name), std::move(data)});
  };

  if (should_ship_manifest_section(profile, options)) {
    add_package_section(venom::package::SectionType::Manifest, "manifest.txt", bytes_from_string(html_manifest(graph, &compiled_routes, "asset-manifest.txt")));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::Integrity, "profile-diagnostics.txt", bytes_from_string(describe_profile(profile)));
  }
  add_package_section(venom::package::SectionType::Integrity, "runtime-policy.vhrd", bytes_from_string(make_runtime_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::Integrity, "package-binding.vbind", bytes_from_string(make_package_binding_metadata(profile, options, package_binding_token, runtime_name, runtime, wasm_name, wasm_bytes, style_name, css, quickjs_engine_name, quickjs_engine_module, quickjs_wasm_name, quickjs_wasm_bytes, worker_name, worker_runtime)));
  add_package_section(venom::package::SectionType::Integrity, "release-profile.vrpf", bytes_from_string(make_release_profile_metadata(profile, options, release_policy, js_bridge.chunks.size())));
  add_package_section(venom::package::SectionType::Integrity, "remote-vendors.vrvd", bytes_from_string(make_remote_vendor_metadata(js_bridge.remote_vendors, remote_vendor_options.offline, remote_vendor_options.cache_directory, vendor_lock_path, vendor_lock_digest, vendor_lock_mode)));
  add_package_section(venom::package::SectionType::QuickJsReleaseFailClosed, "quickjs-release-build-policy.vqbp", bytes_from_string(make_release_build_policy_metadata(profile, options, release_policy, js_bridge.chunks.size())));
  add_package_section(venom::package::SectionType::HostBridge, "host-calls.vhcb", bytes_from_string(make_host_bridge_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::FetchBridge, "fetch-bridge.vfch", bytes_from_string(make_fetch_bridge_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TimerBridge, "timer-bridge.vtmr", bytes_from_string(make_timer_bridge_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::EventQueue, "event-queue.vevq", bytes_from_string(make_event_queue_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsBridge, "quickjs-bridge.vqjs", bytes_from_string(make_quickjs_bridge_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::ScriptIsolation, "script-isolation.vsis", bytes_from_string(make_script_isolation_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::ScriptPolicy, "script-policy.vscp", bytes_from_string(make_script_policy_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsChunks, "quickjs-chunks.vqbc", bytes_from_string(make_quickjs_chunk_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsEngine, "quickjs-engine.vqse", bytes_from_string(make_quickjs_engine_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsEngineModule, "quickjs-engine-module.vqem", bytes_from_string(make_quickjs_engine_module_metadata(profile, options.runtime, js_bridge, quickjs_engine_name, quickjs_wasm_name)));
  add_package_section(venom::package::SectionType::QuickJsWasmRuntime, "quickjs-wasm-runtime.vqwr", bytes_from_string(make_quickjs_wasm_runtime_metadata(profile, options.runtime, quickjs_wasm_name)));
  add_package_section(venom::package::SectionType::QuickJsEngineBackend, "quickjs-wasm-execution.vqwe", bytes_from_string(make_quickjs_wasm_execution_metadata(profile, options, release_policy, js_bridge, quickjs_wasm_name)));
  add_package_section(venom::package::SectionType::QuickJsSourceTransfer, "quickjs-source-transfer.vqst", bytes_from_string(make_quickjs_source_transfer_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsConsoleBridge, "quickjs-console-bridge.vqcb", bytes_from_string(make_quickjs_console_bridge_metadata(profile, options.runtime)));
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsExecutionRecords, "quickjs-execution-records.vqer", bytes_from_string(make_quickjs_execution_records_metadata(profile, options.runtime, js_bridge)));
  }
  add_package_section(venom::package::SectionType::QuickJsResultBridge, "quickjs-result-bridge.vqrb", bytes_from_string(make_quickjs_result_bridge_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsFallbackPolicy, "quickjs-fallback-policy.vqfp", bytes_from_string(make_quickjs_fallback_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsEngineBackend, "quickjs-engine-backend.vqeb", bytes_from_string(make_quickjs_engine_backend_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsNativeParity, "quickjs-native-parity.vqnp", bytes_from_string(make_quickjs_native_parity_metadata(profile, options.runtime, js_bridge, qjs_probe)));
  add_package_section(venom::package::SectionType::QuickJsExecutionMode, "quickjs-execution-mode.vqxm", bytes_from_string(make_quickjs_execution_mode_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsRuntimeAbi, "quickjs-runtime-abi.vqra", bytes_from_string(make_quickjs_runtime_abi_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsHostImports, "quickjs-host-imports.vqhi", bytes_from_string(make_quickjs_host_imports_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsHeapLimits, "quickjs-heap-limits.vqhl", bytes_from_string(make_quickjs_heap_limits_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsScriptBuffer, "quickjs-script-buffer.vqsb", bytes_from_string(make_quickjs_script_buffer_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsConsoleAbi, "quickjs-console-abi.vqca", bytes_from_string(make_quickjs_console_abi_metadata(profile, options.runtime)));
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsParityProbe, "quickjs-parity-probe.vqpp", bytes_from_string(make_quickjs_parity_probe_metadata(profile, options.runtime, qjs_probe)));
  }
  }
  add_package_section(venom::package::SectionType::QuickJsReleaseFallback, "quickjs-release-fallback.vqrf", bytes_from_string(make_quickjs_release_fallback_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsBytecodeManifest, "quickjs-bytecode-manifest.vqbm", bytes_from_string(make_quickjs_bytecode_manifest_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsModuleResolver, "quickjs-module-resolver.vqmr", bytes_from_string(make_quickjs_module_resolver_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsExceptionAbi, "quickjs-exception-abi.vqex", bytes_from_string(make_quickjs_exception_abi_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsHostTrapPolicy, "quickjs-host-trap-policy.vqtp", bytes_from_string(make_quickjs_host_trap_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsExecutionLifecycle, "quickjs-execution-lifecycle.vqel", bytes_from_string(make_quickjs_execution_lifecycle_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsScriptBufferPolicy, "quickjs-script-buffer-policy.vqsp", bytes_from_string(make_quickjs_script_buffer_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsContextLimitPolicy, "quickjs-context-limit-policy.vqlp", bytes_from_string(make_quickjs_context_limit_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsHostCallDispatch, "quickjs-host-call-dispatch.vqhd", bytes_from_string(make_quickjs_host_call_dispatch_metadata(profile, options.runtime)));
  if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsParityContract, "quickjs-parity-contract.vqpc", bytes_from_string(make_quickjs_parity_contract_metadata(profile, options.runtime)));
  }
  add_package_section(venom::package::SectionType::QuickJsReleaseFailClosed, "quickjs-release-failclosed.vqfc", bytes_from_string(make_quickjs_release_failclosed_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsModuleGraph, "quickjs-module-graph.vqmg", bytes_from_string(make_quickjs_module_graph_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsModuleExecution, "quickjs-module-execution.vqmx", bytes_from_string(make_quickjs_module_execution_metadata(profile, options.runtime, js_bridge)));
  if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsModuleCache, "quickjs-module-cache.vqmc", bytes_from_string(make_quickjs_module_cache_metadata(profile, options.runtime, js_bridge)));
  }
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsResolverAudit, "quickjs-resolver-audit.vqra", bytes_from_string(make_quickjs_resolver_audit_metadata(profile, options.runtime, js_bridge)));
  }
  }
  add_package_section(venom::package::SectionType::QuickJsInteropFallback, "quickjs-interop-fallback.vqif", bytes_from_string(make_quickjs_interop_fallback_metadata(profile, options.runtime)));
  if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsExecutionPipeline, "quickjs-execution-pipeline.vqxp", bytes_from_string(make_quickjs_execution_pipeline_metadata(profile, options.runtime, js_bridge)));
  }
  add_package_section(venom::package::SectionType::QuickJsScriptRecords, "quickjs-script-records.vqsr", bytes_from_string(make_quickjs_script_records_metadata(profile, options.runtime, js_bridge)));
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsEvalResults, "quickjs-eval-results.vqev", bytes_from_string(make_quickjs_eval_results_metadata(profile, options.runtime)));
  }
  }
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsConsoleCapture, "quickjs-console-capture.vqcc", bytes_from_string(make_quickjs_console_capture_metadata(profile, options.runtime)));
  }
  }
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsFailureReports, "quickjs-failure-reports.vqfr", bytes_from_string(make_quickjs_failure_reports_metadata(profile, options.runtime)));
  }
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsExecutionJournal, "quickjs-execution-journal.vqxj", bytes_from_string(make_quickjs_execution_journal_metadata(profile, options.runtime, js_bridge)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsCheckpointPolicy, "quickjs-checkpoint-policy.vqcp", bytes_from_string(make_quickjs_checkpoint_policy_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsReplayCursor, "quickjs-replay-cursor.vqrc", bytes_from_string(make_quickjs_replay_cursor_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsResumeState, "quickjs-resume-state.vqrs", bytes_from_string(make_quickjs_resume_state_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsDeterminismAudit, "quickjs-determinism-audit.vqda", bytes_from_string(make_quickjs_determinism_audit_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsSnapshotPolicy, "quickjs-snapshot-policy.vqsk", bytes_from_string(make_quickjs_snapshot_policy_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsSnapshotRecords, "quickjs-snapshot-records.vqsn", bytes_from_string(make_quickjs_snapshot_records_metadata(profile, options.runtime, js_bridge)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsReplayValidation, "quickjs-replay-validation.vqrv", bytes_from_string(make_quickjs_replay_validation_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsDeterminismLedger, "quickjs-determinism-ledger.vqdl", bytes_from_string(make_quickjs_determinism_ledger_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsAuditSeal, "quickjs-audit-seal.vqas", bytes_from_string(make_quickjs_audit_seal_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsExecutionCommit, "quickjs-execution-commit.vqxc", bytes_from_string(make_quickjs_execution_commit_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsRollbackPolicy, "quickjs-rollback-policy.vqrp", bytes_from_string(make_quickjs_rollback_policy_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsHostCallReceipts, "quickjs-host-call-receipts.vqhr", bytes_from_string(make_quickjs_host_call_receipts_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsReleaseAcceptance, "quickjs-release-acceptance.vqac", bytes_from_string(make_quickjs_release_acceptance_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsCommitAudit, "quickjs-commit-audit.vqca", bytes_from_string(make_quickjs_commit_audit_metadata(profile, options.runtime)));
  }
  add_package_section(venom::package::SectionType::QuickJsCapabilityPolicy, "quickjs-capability-policy.vqcpol", bytes_from_string(make_quickjs_capability_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsHostIoPolicy, "quickjs-host-io-policy.vqio", bytes_from_string(make_quickjs_host_io_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsPermissionSeal, "quickjs-permission-seal.vqps", bytes_from_string(make_quickjs_permission_seal_metadata(profile, options.runtime)));
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsPolicyReceipts, "quickjs-policy-receipts.vqpr", bytes_from_string(make_quickjs_policy_receipts_metadata(profile, options.runtime)));
  }
  add_package_section(venom::package::SectionType::QuickJsReleaseGate, "quickjs-release-gate.vqrg", bytes_from_string(make_quickjs_release_gate_metadata(profile, options.runtime)));
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsHostIoDecision, "quickjs-host-io-decision.vqid", bytes_from_string(make_quickjs_host_io_decision_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsHostIoDenyTrace, "quickjs-host-io-deny-trace.vqdt", bytes_from_string(make_quickjs_host_io_deny_trace_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsCapabilityLedger, "quickjs-capability-ledger.vqclg", bytes_from_string(make_quickjs_capability_ledger_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsPolicySealAudit, "quickjs-policy-seal-audit.vqpsa", bytes_from_string(make_quickjs_policy_seal_audit_metadata(profile, options.runtime)));
  }
  add_package_section(venom::package::SectionType::QuickJsRuntimeDenylist, "quickjs-runtime-denylist.vqrd", bytes_from_string(make_quickjs_runtime_denylist_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsContextLifecycle, "quickjs-context-lifecycle.vqcl", bytes_from_string(make_quickjs_context_lifecycle_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::HostCapabilities, "host-capabilities.vhcap", bytes_from_string(make_host_capabilities_metadata(profile, options.runtime, js_bridge)));
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsAdapterDiagnostics, "quickjs-adapter-diagnostics.vqad", bytes_from_string(make_quickjs_adapter_diagnostics_metadata(profile, options.runtime, js_bridge, quickjs_engine_name)));
  }
  add_package_section(venom::package::SectionType::ScriptEnginePolicy, "script-engine-policy.vsep", bytes_from_string(make_script_engine_policy_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::AsyncHostQueue, "async-host-queue.vahq", bytes_from_string(make_async_host_queue_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::Routes, "routes.vbrt", compiled_routes.route_table);
  add_package_section(venom::package::SectionType::Css, style_name, bytes_from_string(css));
  if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::JavaScript, "scripts.vjsb", js_bridge.bundle);
  }
  if (!js_bridge.bridge_registry_bytecode.empty()) {
    auto protected_registry = encode_protected_bridge_registry(js_bridge, remote_vendor_options.bridge_id_salt);
    if (protected_registry.empty() || count_marker(protected_registry, "VQJSE006") != 1u) {
      throw std::runtime_error("protected bridge registry was not emitted as exactly one VQJSE006 record");
    }
    add_package_section(venom::package::SectionType::JavaScript,
                        "protected-bridge-registry.vqbc",
                        std::move(protected_registry));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::JavaScript, "script-diagnostics.txt", bytes_from_string(js_bridge.diagnostics));
    add_package_section(venom::package::SectionType::JavaScript, "bundle-preview.js", bytes_from_string(js_preview));
  }
  add_package_section(venom::package::SectionType::Strings, "strings.vstr", compiled_routes.string_table);
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::DomTemplates, "dom-templates.txt", bytes_from_string(compiled_routes.preview));
  }
  add_package_section(venom::package::SectionType::VmBytecode, "route-bytecode.vmbc", compiled_routes.bytecode);

  std::unordered_map<std::string, std::vector<JsChunk>> lazy_scripts_by_route;
  for (const auto& chunk : js_bridge.chunks) {
    lazy_scripts_by_route[chunk.route].push_back(chunk);
  }
  std::vector<LazySectionPlanRow> lazy_rows;
  lazy_rows.reserve(compiled_routes.routes.size());
  for (const auto& route : compiled_routes.routes) {
    LazySectionPlanRow row;
    row.route = route.route;
    row.instruction_count = route.instruction_count;
    if (compiled_routes.routes.size() == 1u) {
      row.route_section = "route-bytecode.vmbc";
      row.route_bytecode_size = static_cast<std::uint32_t>(compiled_routes.bytecode.size());
    } else {
      row.route_section = lazy_route_section_name(route.route);
      auto route_bytes = make_single_route_bytecode_section(route.program, poly);
      row.route_bytecode_size = static_cast<std::uint32_t>(route_bytes.size());
      add_package_section(venom::package::SectionType::VmBytecode, row.route_section, std::move(route_bytes));
    }

    const auto scripts_it = lazy_scripts_by_route.find(route.route);
    if (scripts_it != lazy_scripts_by_route.end() && !scripts_it->second.empty()) {
      row.script_section = lazy_script_section_name(route.route);
      row.script_count = static_cast<std::uint32_t>(scripts_it->second.size());
      add_package_section(venom::package::SectionType::JavaScript, row.script_section, encode_route_script_bundle(scripts_it->second, profile.kind == ProfileKind::Prod || profile.strip_debug_metadata));
    }
    lazy_rows.push_back(std::move(row));
  }

  add_package_section(venom::package::SectionType::Integrity, "vm-polymorph.vpol", poly.encode_binary());
  if (profile.kind == ProfileKind::Prod || profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::Integrity,
                        "release-diversification.vrd3",
                        make_release_diversification_table(poly));
    add_package_section(venom::package::SectionType::Integrity,
                        "quickjs-abi-fingerprint.vqaf",
                        make_quickjs_abi_fingerprint());
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJs, "quickjs-probe.txt", bytes_from_string(qjs_probe));
    add_package_section(venom::package::SectionType::QuickJs, "quickjs-bridge-plan.txt", bytes_from_string("QuickJS native embedding is available for compile-time probes. Browser QuickJS execution now loads the v0.46 bytecode-record pipeline with the release-policy-gated QuickJS/WASM host-I/O decision enforcement boundary with ABI-table-driven route-scoped script context records, heap/accounting limits, script-buffer allocation, console callback ABI, and parity probes, bytecode manifests, module resolver contracts, exception records, and host-trap policy; the AEAD provider status remains honest.\n"));
  }
  add_package_section(venom::package::SectionType::AssetManifest, "asset-manifest.txt", bytes_from_string(assets.manifest_text));

  for (const auto& file : graph.files) {
    const auto rel = file.relative;
    const auto lower = [&]() {
      std::string value = rel;
      std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
      return value;
    }();
    const bool browser_manifest = rel == "venom.browser.json";
    const bool notice = lower == "third_party_notices.txt" || lower == "third-party-notices.txt" ||
                        (lower.size() >= 24 && lower.compare(lower.size() - 24, 24, "/third_party_notices.txt") == 0) ||
                        (lower.size() >= 24 && lower.compare(lower.size() - 24, 24, "/third-party-notices.txt") == 0);
    if (!is_code_or_markup(file) && (!production_hardening || (!browser_manifest && !notice))) {
      add_package_section(venom::package::SectionType::Asset, file.relative, file.bytes);
    }
  }

  constexpr std::uint32_t kPolymorphicLayoutMaxPadding = 31u;
  add_package_section(venom::package::SectionType::Integrity,
                      "package-layout.vlay",
                      bytes_from_string(make_package_layout_metadata(profile, options, poly, package_sections, kPolymorphicLayoutMaxPadding)));

  add_package_section(venom::package::SectionType::Integrity,
                      "lazy-sections.vlazy",
                      bytes_from_string(make_lazy_sections_metadata(profile, options, lazy_rows)));

  add_package_section(venom::package::SectionType::PackageFeatures,
                      "package-features.vfeat",
                      bytes_from_string(make_package_features_metadata(profile, options.runtime, package_sections, options.crypto_provider)));

  if (profile.integrity_metadata) {
    add_package_section(venom::package::SectionType::Integrity,
                        "integrity-auth.vsig",
                        bytes_from_string(make_integrity_metadata(profile, options.runtime, package_sections, options.crypto_provider)));
  }

  if (profile.shuffle_sections) {
    shuffle_package_sections(package_sections, poly);
  }

  trace.step("Assemble encrypted and compressed VBC package sections");
  venom::package::Writer writer;
  writer.set_flags(package_flags);
  writer.set_compression_enabled(profile.compress_sections);
  writer.set_section_encryption_enabled(profile.crypto_provider_ready && profile.kind != ProfileKind::Dev);
  writer.set_section_name_redaction_enabled(profile.strip_debug_metadata && profile.kind != ProfileKind::Dev);
  writer.set_layout_polymorphism_enabled(profile.polymorphic && profile.kind != ProfileKind::Dev);
  writer.set_layout_seed(poly.seed);
  writer.set_layout_max_padding(kPolymorphicLayoutMaxPadding);
  writer.set_section_crypto_provider(selected_writer_crypto_provider(options.crypto_provider));
  for (auto& section : package_sections) {
    writer.add_section(section.type, section.flags, std::move(section.name), std::move(section.data));
  }

  if (hardened_release_asset) {
    std::filesystem::create_directories(assets_dir / "app");
    std::filesystem::create_directories(assets_dir / "style");
    std::filesystem::create_directories(assets_dir / "loader");
    std::filesystem::create_directories(assets_dir / "runtime");
    std::filesystem::create_directories(assets_dir / "workers");
  }
  const auto package_temp_path = hardened_release_asset ? (assets_dir / "app" / ".app.vbc.tmp") : (assets_dir / ".app.vbc.tmp");
  trace.detail("package sections queued: " + std::to_string(package_sections.size()));
  trace.step("Write and verify polymorphic package container");
  writer.write(package_temp_path);
  const auto package_bytes = read_bytes(package_temp_path);
  const auto package_probe = venom::package::read_package(package_temp_path);
  const auto package_file_name = named_output("app", ".vbc", package_bytes, options.hashed_assets);
  const auto package_name = hardened_release_asset ? "app/" + package_file_name : package_file_name;
  const auto package_path = assets_dir / package_name;
  std::filesystem::rename(package_temp_path, package_path);

  const auto loader_runtime_ref = hardened_release_asset ? "../" + runtime_name : runtime_name;
  const auto loader_package_ref = hardened_release_asset ? "../" + package_name : package_name;
  const auto loader_engine_ref = hardened_release_asset ? "../" + quickjs_engine_name : quickjs_engine_name;
  const auto loader_qjs_wasm_ref = hardened_release_asset ? "../" + quickjs_wasm_name : quickjs_wasm_name;
  const auto loader_runtime_wasm_ref = hardened_release_asset ? "../" + wasm_name : wasm_name;
  const auto loader_style_ref = hardened_release_asset ? "../" + style_name : style_name;
  const auto loader_worker_ref = hardened_release_asset ? "../" + worker_name : std::string{};
  auto loader = make_loader_js(loader_runtime_ref, loader_package_ref, package_probe.package_hash, venom::package::sha256_hex(package_bytes), profile.runtime_hardened || profile.fail_closed, loader_engine_ref, loader_qjs_wasm_ref, loader_runtime_wasm_ref, loader_style_ref, package_binding_token, loader_worker_ref, venom::package::sha256_hex(quickjs_wasm_bytes), js_bridge.protected_exports,
      bridge_invoke_opcode, bridge_cancel_opcode, bridge_result_opcode, bridge_error_opcode);
  if (production_hardening) {
    trace.step("Harden and validate bootstrap loader");
    loader = harden_release_js_asset(std::move(loader));
    loader = ast_harden_release_js("loader", loader);
    validate_protected_js_asset("loader", loader);
  }
  const auto loader_file_name = named_output("loader", ".js", loader, options.hashed_assets);
  const auto loader_name = hardened_release_asset ? "loader/" + loader_file_name : loader_file_name;
  const auto html = make_bootstrap_html(graph,
                                        "assets/" + loader_name,
                                        "assets/" + style_name,
                                        venom::package::sha256_sri(bytes_from_string(loader)),
                                        venom::package::sha256_sri(bytes_from_string(css)));

  trace.step("Emit distribution assets and integrity-bound bootstrap");
  write_text(options.output / "index.html", html);
  if (!hardened_release_asset) {
    emit_html_route_shells(graph, options.output, loader_name, style_name);
    write_text(options.output / "sw.js", make_service_worker_js());
    write_text(options.output / "favicon.ico", "");
  }
  write_text(assets_dir / style_name, css);
  write_text(assets_dir / loader_name, loader);
  write_text(assets_dir / runtime_name, runtime);
  write_text(assets_dir / quickjs_engine_name, quickjs_engine_module);
  if (hardened_release_asset) write_text(assets_dir / worker_name, worker_runtime);
  {
    std::ofstream qjs_wasm_out(assets_dir / quickjs_wasm_name, std::ios::binary);
    if (!qjs_wasm_out) {
      throw std::runtime_error("failed to write " + (assets_dir / quickjs_wasm_name).string());
    }
    qjs_wasm_out.write(reinterpret_cast<const char*>(quickjs_wasm_bytes.data()), static_cast<std::streamsize>(quickjs_wasm_bytes.size()));
    qjs_wasm_out.flush();
    if (!qjs_wasm_out) {
      throw std::runtime_error("failed to finish writing " + (assets_dir / quickjs_wasm_name).string());
    }
  }
  {
    const auto written_quickjs_wasm = read_bytes(assets_dir / quickjs_wasm_name);
    if (written_quickjs_wasm != quickjs_wasm_bytes) {
      throw std::runtime_error("QuickJS WASM write verification failed");
    }
    const auto written_digest = venom::package::sha256_hex(written_quickjs_wasm);
    const auto expected_digest = venom::package::sha256_hex(quickjs_wasm_bytes);
    if (written_digest != expected_digest) {
      throw std::runtime_error("QuickJS WASM digest changed after package binding generation");
    }
  }
  if (wasm_runtime) {
    std::ofstream wasm_out(assets_dir / wasm_name, std::ios::binary);
    if (!wasm_out) {
      throw std::runtime_error("failed to write " + (assets_dir / wasm_name).string());
    }
    wasm_out.write(reinterpret_cast<const char*>(wasm_bytes.data()), static_cast<std::streamsize>(wasm_bytes.size()));
  }
  if (should_emit_external_asset_manifest(profile, options)) {
    write_text(assets_dir / "asset-manifest.txt", assets.manifest_text);
  }
  emit_public_assets(assets, assets_dir);

  trace.step("Reopen output and verify package, hashes, and runtime artifacts");
  const auto validated_package = venom::package::read_package(package_path);
  if (!vendor_lock.present || options.refresh_vendors) {
    try {
      write_remote_vendor_lock(vendor_lock_path, js_bridge.remote_vendors);
    } catch (...) {
      std::error_code cleanup_error;
      std::filesystem::remove_all(options.output, cleanup_error);
      throw;
    }
  }
  trace.step("Write provenance metadata and private build reports");
  std::ostringstream provenance;
  provenance << "{\n"
             << "  \"schema_version\": 1,\n"
             << "  \"product\": \"" << json_escape(VENOM_PRODUCT_NAME) << "\",\n"
             << "  \"venom_version\": \"" << VENOM_VERSION_STRING << "\",\n"
             << "  \"package_format_version\": " << validated_package.version << ",\n"
             << "  \"runtime_abi_version\": " << validated_package.runtime_abi << ",\n"
             << "  \"profile\": \"" << json_escape(profile.name) << "\",\n"
             << "  \"security_target\": \"" << json_escape(options.security_target) << "\",\n"
             << "  \"quickjs_backend\": \"" << json_escape(release_policy.backend) << "\",\n"
             << "  \"runtime_modules\": " << runtime_module_plan_json(runtime_modules) << ",\n"
             << "  \"protection_closure\": {\"requested\": " << js_bridge.protected_intents_requested
             << ", \"resolved\": " << js_bridge.protected_intents_resolved
             << ", \"registry_present\": " << (!js_bridge.bridge_registry_bytecode.empty() ? "true" : "false")
             << ", \"whole_file_intents\": " << js_bridge.protected_whole_file_intents
             << ", \"expected_quickjs_records\": " << js_bridge.expected_quickjs_records << "},\n"
             << "  \"module_graph\": {\"chunks\": " << js_bridge.chunks.size()
             << ", \"edges\": " << js_bridge.module_edges.size()
             << ", \"dynamic_literal_edges\": " << std::count_if(js_bridge.module_edges.begin(), js_bridge.module_edges.end(), [](const auto& edge) { return edge.dynamic; }) << "},\n"
             << "  \"vendor_lock_sha256\": \"" << vendor_lock_digest << "\",\n"
             << "  \"package_sha256\": \"" << venom::package::sha256_hex(package_bytes) << "\",\n"
             << "  \"package_asset\": \"assets/" << json_escape(package_name) << "\",\n"
             << "  \"reproducible_timestamp_source\": \"SOURCE_DATE_EPOCH\"\n"
             << "}\n";
  const auto build_metadata_ref = hardened_release_asset ? std::string{"assets/app/build.json"} : std::string{"assets/build.json"};
  const auto build_metadata_path = options.output / build_metadata_ref;
  write_text(build_metadata_path, provenance.str());
  const auto private_report_dir = std::filesystem::absolute(options.output).parent_path() /
      ".venom" / options.output.filename();
  write_text(private_report_dir / "protection-intents.json", js_bridge.protection_intent_ledger_json);
  if (profile.name == "dev") {
    const auto report_dir = options.output / "build" / "reports";
    write_text(report_dir / "execution-plan.txt", js_bridge.execution_plan_text);
    write_text(report_dir / "execution-plan.json", js_bridge.execution_plan_json);
    write_text(report_dir / "function-plan.txt", js_bridge.function_plan_text);
    write_text(report_dir / "function-plan.json", js_bridge.function_plan_json);
    write_text(report_dir / "function-extraction-plan.txt", js_bridge.extraction_plan_text);
    write_text(report_dir / "function-extraction-plan.json", js_bridge.extraction_plan_json);
    write_text(report_dir / "realm-bridge-contract.json", js_bridge.realm_bridge_contract_json);
    write_text(report_dir / "bridge-rewrite-plan.json", js_bridge.bridge_rewrite_report_json);
    if (!js_bridge.protected_api_typescript.empty())
      write_text(options.output / "assets" / "app" / "venom-exports.d.ts", js_bridge.protected_api_typescript);
  }

  trace.step("Finalize protection report");
  if (options.format != OutputFormat::Json && options.verbosity > 0) {
    print_build_protection_report(profile, options, package_path, package_bytes, validated_package, should_emit_external_asset_manifest(profile, options));
  }

  if (options.format == OutputFormat::Json) {
    std::cout << "{\"ok\":true,\"version\":\"" << VENOM_VERSION_STRING
              << "\",\"output\":\"" << json_escape(options.output.generic_string())
              << "\",\"package\":\"assets/" << json_escape(package_name)
              << "\",\"package_sha256\":\"" << venom::package::sha256_hex(package_bytes)
              << "\",\"build_metadata\":\"" << json_escape(build_metadata_ref) << "\"}\n";
    return true;
  }

  trace.success("Protected distribution compiled successfully");
  if (options.verbosity == 0) {
    std::cout << "venom: built protected distribution => " << options.output.string() << "\n";
    return true;
  }
  std::cout << "venom: vendor lock => " << vendor_lock_path.string()
            << " mode=" << vendor_lock_mode
            << " sha256=" << vendor_lock_digest << "\n"
            << "venom: built " << graph.files.size() << " files, "
            << graph.routes.size() << " routes, " << assets.records.size() << " assets, "
            << js_bridge.chunks.size() << " script chunks ("
            << compiled_routes.bytecode.size() << " route bytecode bytes) using profile '" << profile.name << "'\n"
            << "venom: package v" << validated_package.version << " abi " << validated_package.runtime_abi
            << " sections " << validated_package.sections.size()
            << " bytes " << validated_package.file_size << "\n"
            << "venom: dist assets => " << loader_name << ", " << runtime_name << ", " << package_name << ", " << style_name << ", " << quickjs_engine_name << ", " << quickjs_wasm_name << (hardened_release_asset ? ", " + worker_name : "") << (hardened_release_asset ? "" : ", sw.js");
  if (wasm_runtime) {
    std::cout << ", " << wasm_name;
  }
  std::cout << "\n"
            << "venom: profile => compressed=" << (profile.compress_sections ? "yes" : "no")
            << " fail_closed=" << (profile.fail_closed ? "yes" : "no")
            << " integrity_provider_ready=" << (profile.integrity_metadata ? "yes" : "no")
            << " integrity_metadata=" << (profile.integrity_metadata ? "yes" : "no")
            << " runtime_hardened=" << (profile.runtime_hardened ? "yes" : "no")
            << " strip_debug_metadata=" << (profile.strip_debug_metadata ? "yes" : "no")
            << " sealed_sections=" << ((profile.crypto_provider_ready && profile.kind != ProfileKind::Dev) ? "yes" : "no")
            << " crypto_provider=" << selected_section_sealer_name(profile, options.crypto_provider)
            << " external_debug_manifest=" << (should_emit_external_asset_manifest(profile, options) ? "yes" : "no")
            << " aead_provider_ready=" << (profile.aead_provider_ready ? "yes" : "no")
            << " signature_provider_ready=no"
            << " runtime=" << options.runtime
            << " quickjs_backend=" << release_policy.backend
            << " host_fallback=" << (release_policy.fallback_allowed ? "allowed" : "denied")
            << " release_policy=" << release_policy.decision << "\n"
            << "venom: quickjs probe => " << qjs_probe << "\n"
            << "venom: output => " << options.output.string() << "\n";
  return true;
}

} // namespace venom::compiler

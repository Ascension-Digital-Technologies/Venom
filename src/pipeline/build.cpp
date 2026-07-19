#include "venom/base/error.hpp"
#include "venom/internal/pipeline/runtime_modules.hpp"
#include "venom/pipeline/build.hpp"
#include "venom/internal/pipeline/chrome_extension.hpp"
#include "venom/internal/pipeline/build_support.hpp"
#include "venom/internal/pipeline/build_package_metadata.hpp"
#include "venom/internal/pipeline/planning/package_plan.hpp"
#include "venom/internal/pipeline/planning/section_plan.hpp"
#include "venom/internal/pipeline/build_runtime_metadata.hpp"
#include "venom/internal/pipeline/build_report.hpp"
#include "venom/core/version.hpp"
#include "venom/pipeline/planner.hpp"
#include "venom/core/console.hpp"
#include "venom/internal/pipeline/project_ir.hpp"
#include "venom/internal/pipeline/assets.hpp"
#include "venom/internal/pipeline/capability_analysis.hpp"
#include "venom/internal/pipeline/css.hpp"
#include "venom/internal/pipeline/html.hpp"
#include "venom/pipeline/js.hpp"
#include "venom/internal/pipeline/native_js_hardener.hpp"
#include "venom/frontends/typescript/frontend.hpp"
#include "venom/core/profile.hpp"
#include "venom/core/process.hpp"
#include "venom/pipeline/security.hpp"
#include "venom/core/site.hpp"
#include "venom/package/hash.hpp"
#include "venom/package/crypto.hpp"
#include "venom/package/reader.hpp"
#include "venom/package/writer.hpp"
#include "venom/quickjs/abi.hpp"
#include "venom/quickjs/bytecode.hpp"
#include "venom/quickjs/bridge.hpp"
#include "venom/quickjs/engine.hpp"
#include "venom/vm/polymorph.hpp"
#include "venom/vm/encoder.hpp"
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
using namespace build_report_detail;
bool should_emit_external_asset_manifest(const Profile&, const BuildOptions& options) {
  return options.emit_debug_manifest;
}


} // namespace

bool build_site(const BuildOptions& requested_options) {
  const BuildOptions& options = requested_options;
  Console trace(options);
  const auto project_name = std::filesystem::absolute(options.input).filename().string();
  trace.banner("Building " + project_name, "protected " + options.profile + " distribution with QuickJS/WASM");
  trace.field("Input", std::filesystem::absolute(options.input).string());
  trace.field("Output", std::filesystem::absolute(options.output).string());
  trace.field("Profile", options.profile);
  trace.field("Target", options.project_target);
  trace.field("Protection", options.protection_level);
  trace.field("Runtime", options.runtime + " / " + options.quickjs_backend);
  trace.field("Cache", options.cache_enabled ? "enabled" : "disabled");
  trace.phase("Validate build policy and load configuration");
  enforce_build_protection_plan(options);
  trace.info("Loaded " + options.profile + " profile with " + options.protection_level + " protection and " + (options.cache_enabled ? "incremental caching" : "caching disabled"));
  if (!options.key_file.empty()) {
    load_package_key_file_for_process(options.key_file);
  }
  if (options.require_audited_crypto && options.crypto_provider != "libsodium") {
    raise_error("VENOM-E5000", "--require-audited-crypto is not available for browser dev/prod builds");
  }
  const auto profile = resolve_profile(options.profile);
  trace.detail("profile flags: fail_closed=" + std::string(profile.fail_closed ? "yes" : "no") +
               " polymorphic=" + std::string(profile.polymorphic ? "yes" : "no") +
               " strip_debug=" + std::string(profile.strip_debug_metadata ? "yes" : "no"));
  trace.phase("Verify embedded QuickJS runtime and discover site graph");
  require_real_embedded_quickjs_runtime();
  const auto graph = collect_site(options.input);
  const bool chrome_extension_target = chrome_extension::is_extension_target(options.project_target);
  if (chrome_extension_target) {
    chrome_extension::validate_project(graph);
    const auto extension_analysis = chrome_extension::analyze_project(graph);
    trace.detail("extension compatibility: " + chrome_extension::compatibility_summary(extension_analysis));
    for (const auto& warning : extension_analysis.warnings) trace.detail("extension warning: " + warning);
  }
  auto project_ir = make_project_ir(graph);
  const auto compiler_cache_root = options.cache_directory.empty()
      ? (std::filesystem::absolute(options.output).parent_path() / ".venom-cache" / graph.root.filename() / "compiler-v13")
      : std::filesystem::absolute(options.cache_directory);
  if (options.cache_enabled) {
    write_project_ir_atomic(project_ir, compiler_cache_root / "project-ir.json");
  }
  reset_hardener_cache_stats();
  native_js_hardener::reset_runtime_stats();
  venom::quickjs::reset_bytecode_cache_stats();
  typescript::reset_cache_stats();
  configure_hardener_cache(options.cache_enabled, compiler_cache_root / "hardener", options.verbosity > 1);
  venom::quickjs::configure_bytecode_cache(options.cache_enabled, compiler_cache_root / "quickjs-bytecode", options.verbosity > 1);
  typescript::configure_cache(options.cache_enabled, compiler_cache_root / "typescript", options.verbosity > 1);
  trace.info("Discovered " + std::to_string(graph.files.size()) + " files across " + std::to_string(graph.routes.size()) + " HTML routes");
  trace.detail("project IR v" + std::to_string(project_ir.version) + ": " + project_ir.project_fingerprint);
  trace.detail(std::string("incremental cache: ") + (options.cache_enabled ? compiler_cache_root.string() : "disabled"));

  if (graph.routes.empty()) {
    raise_error("VENOM-E5000", "site has no html routes");
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
    } else if (false && options.cache_enabled) {
      // Development builds use a project-derived identity so unchanged modules
      // produce byte-for-byte identical QuickJS cache keys across warm builds.
      remote_vendor_options.bridge_id_salt = std::string{"dev-cache:"} + project_ir.project_fingerprint;
    } else {
      const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
      remote_vendor_options.bridge_id_salt = std::string{"nonce:"} + std::to_string(now);
    }
  }
  trace.phase("Analyze JavaScript runtimes, modules, annotations, and dependencies");
  auto js_bridge = compile_js_bridge(graph, remote_vendor_options, false);
  trace.info("Planned " + std::to_string(js_bridge.chunks.size()) + " protected script chunks and " +
             std::to_string(js_bridge.protected_exports.size()) + " protected exports");
  trace.detail("module graph: " + std::to_string(js_bridge.module_edges.size()) + " edges; remote vendors: " +
               std::to_string(js_bridge.remote_vendors.size()));
  enrich_project_ir(project_ir, js_bridge);
  if (options.cache_enabled) write_project_ir_atomic(project_ir, compiler_cache_root / "project-ir.json");
  trace.detail(project_ir_summary(project_ir));
  require_embedded_quickjs_module_bundle_runtime(js_bridge);
  if (options.vendor_offline && !vendor_lock.present && !js_bridge.remote_vendors.empty()) {
    raise_error("VENOM-E5000", "offline remote vendoring requires venom.lock; run one reviewed online build first");
  }
  validate_remote_vendor_lock_coverage(vendor_lock, js_bridge.remote_vendors, options.refresh_vendors);
  trace.phase("Compile and validate protected QuickJS bytecode records");
  // Validate every protected script/bytecode record before touching the output
  // directory. A capacity failure must never leave a partial production dist.
  const auto validated_bytecode_records = make_quickjs_bytecode_records(js_bridge, profile.kind == ProfileKind::Prod || profile.strip_debug_metadata);
  trace.info("Validated protected execution records for " + std::to_string(js_bridge.protected_exports.size()) +
             " public exports (" + std::to_string(validated_bytecode_records.size()) + " inline, " +
             std::to_string(js_bridge.bridge_registry_chunks.size()) + " lazy registry chunks)");
  const auto vendor_lock_digest = remote_vendor_lock_sha256(js_bridge.remote_vendors);
  const std::string vendor_lock_mode = options.refresh_vendors
      ? "refreshed"
      : (vendor_lock.present ? "enforced" : "generated");
  const auto release_policy = resolve_release_build_policy(profile, options, js_bridge.chunks.size());
  enforce_release_build_policy(release_policy, js_bridge.chunks.size());

  const bool release_trust_domain = profile.kind == ProfileKind::Prod || options.strict_release;
  const bool hardened_release_asset = profile.fail_closed || release_trust_domain;
  const bool production_hardening = profile.name == "prod";
  if (hardened_release_asset) {
    const auto bytecode_format = wasm_truth_value("bytecode_format", "");
    const auto native_reader = wasm_truth_value("native_object_reader", "");
    const auto source_materialization = wasm_truth_value("source_materialization", "");
    if (bytecode_format != "VQJSBC03" || native_reader != "JS_ReadObject" || source_materialization != "false") {
      raise_error("VENOM-E5000", 
          "embedded QuickJS WASM is legacy or incompatible with native VQJSBC03 records; "
          "rebuild and embed it with scripts/linux/build-emsdk.sh or scripts/windows/build-emsdk.bat before producing a protected release");
    }
  }

  trace.phase("Prepare clean distribution layout");
  std::filesystem::remove_all(options.output);
  const auto assets_dir = options.output / "assets";
  std::filesystem::create_directories(assets_dir);
  trace.info("Reset output directory and created the production asset layout");

  trace.phase("Process static assets and bundle stylesheets");
  const auto assets = build_asset_pipeline(graph, options.hashed_assets, production_hardening, chrome_extension_target);
  const auto css = bundle_css(graph, assets);
  const bool wasm_runtime = options.runtime == "wasm";
  if (options.runtime != "js" && options.runtime != "wasm") {
    raise_error("VENOM-E5000", "unsupported runtime mode: " + options.runtime);
  }
  const auto deterministic_seed = options.diversification_seed != 0u ? options.diversification_seed : (profile.deterministic ? 0xC0FFEEu : 0u);
  const auto poly = venom::vm::make_polymorphic_plan(deterministic_seed, profile.polymorphic);
  trace.info("Prepared " + std::to_string(assets.records.size()) + " public assets and bundled " + std::to_string(css.size()) + " CSS bytes");
  trace.phase("Plan runtime capabilities and build-specific polymorphism");
  const auto capability_graph = analyze_capabilities(graph.root);
  const auto runtime_modules = plan_runtime_modules(graph, capability_graph, options);
  const auto wasm_bytes = wasm_runtime ? make_runtime_wasm_module() : std::vector<unsigned char>{};
  const auto wasm_file_name = wasm_runtime ? named_output(hardened_release_asset ? "" : "runtime", ".wasm", wasm_bytes, options.hashed_assets) : std::string{};
  const auto wasm_name = hardened_release_asset && wasm_runtime ? "wasm/" + wasm_file_name : wasm_file_name;
  // The runtime bridge resolves this route against bootOptions.assetBaseUrl.
  // Hardened loaders define that base as the assets/ directory, so the package-owned
  // route must be assets-root-relative (wasm/<hash>.wasm), not module-relative
  // (../wasm/<hash>.wasm). Using the latter escapes assets/ and produces /wasm/* 404s.
  const auto wasm_runtime_asset_ref = hardened_release_asset ? wasm_name : wasm_file_name;
  trace.info("Selected " + std::to_string(runtime_modules.enabled_modules().size()) + " runtime modules with " + std::to_string(capability_graph.occurrences.size()) + " detected capabilities");
  trace.detail("polymorphic seed: " + std::to_string(poly.seed));
  trace.phase("Generate runtime bridge and compile HTML route bytecode");
  auto runtime = wasm_runtime ? make_runtime_wasm_bridge_js(graph, wasm_runtime_asset_ref, hardened_release_asset, &poly) : make_runtime_js(graph, hardened_release_asset);
  runtime = specialize_runtime_modules(std::move(runtime), runtime_modules);
  const auto js_preview = js_bridge.preview.empty() ? bundle_js_preview(graph) : js_bridge.preview;
  const auto compiled_routes = compile_html_routes(graph, poly);
  trace.info("Generated the browser bridge and compiled " + std::to_string(compiled_routes.routes.size()) + " HTML route payloads");

  trace.phase("Prepare QuickJS engine and WASM runtime assets");
  venom::quickjs::Engine engine;
  const auto qjs_probe = engine.eval_string("'quickjs:' + (1 + 1)");

  const auto style_file_name = named_output(hardened_release_asset ? "" : "style", ".css", css, options.hashed_assets);
  const auto style_name = hardened_release_asset ? "app/" + style_file_name : style_file_name;
  auto quickjs_engine_module = make_quickjs_engine_module_js(release_trust_domain);
  auto quickjs_wasm_bytes = make_quickjs_runtime_wasm_module();
  trace.info("Prepared QuickJS engine module and " + std::to_string(quickjs_wasm_bytes.size()) + " bytes of WASM runtime data");
  const auto bridge_invoke_opcode = bridge_protocol_opcode(remote_vendor_options.bridge_id_salt, "invoke");
  const auto bridge_cancel_opcode = bridge_protocol_opcode(remote_vendor_options.bridge_id_salt, "cancel");
  const auto bridge_result_opcode = bridge_protocol_opcode(remote_vendor_options.bridge_id_salt, "result");
  const auto bridge_error_opcode = bridge_protocol_opcode(remote_vendor_options.bridge_id_salt, "error");
  trace.phase("Generate protected worker and harden runtime assets");
  const bool external_lazy_registry = options.lazy_loading_enabled && !js_bridge.bridge_registry_chunks.empty();
  std::vector<WorkerLazyRegistryChunk> lazy_registry_chunks;
  if (external_lazy_registry) {
    for (const auto& chunk : js_bridge.bridge_registry_chunks) {
      WorkerLazyRegistryChunk runtime_chunk;
      runtime_chunk.id = chunk.id;
      const auto file_name = named_output("", ".vqc", chunk.bytecode, options.hashed_assets);
      runtime_chunk.asset = "../app/" + file_name;
      runtime_chunk.sha256 = venom::package::sha256_hex(chunk.bytecode);
      runtime_chunk.candidates = chunk.candidates;
      for (const auto& preload_name : options.lazy_preload) {
        const auto export_it = std::find_if(js_bridge.protected_exports.begin(), js_bridge.protected_exports.end(),
            [&](const auto& item) { return item.first == preload_name; });
        if (export_it != js_bridge.protected_exports.end() &&
            std::find(chunk.candidates.begin(), chunk.candidates.end(), export_it->second) != chunk.candidates.end()) {
          runtime_chunk.preload = true;
        }
      }
      lazy_registry_chunks.push_back(std::move(runtime_chunk));
    }
  }
  // Protection-closure verification counts QuickJS records physically stored in
  // the VBC package. Lazy protected registry chunks are emitted as authenticated
  // external .vqc assets and are attested separately, so they must not inflate
  // the package record count. Derive the expected value from the exact chunks
  // that the package serializers will encode instead of from planner intents.
  js_bridge.expected_quickjs_records = static_cast<std::size_t>(std::count_if(
      js_bridge.chunks.begin(), js_bridge.chunks.end(),
      [](const auto& chunk) { return (chunk.flags & JsChunkBytecodeEncoded) != 0u; }));
  if (!external_lazy_registry && !js_bridge.bridge_registry_bytecode.empty()) {
    ++js_bridge.expected_quickjs_records;
  }
  auto worker_runtime = make_worker_runtime_js(js_bridge.bridge_candidate_ids,
      external_lazy_registry ? std::vector<unsigned char>{} : js_bridge.bridge_registry_bytecode,
      lazy_registry_chunks,
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
    runtime = ast_harden_release_js("runtime", runtime, poly.seed_commitment);
    validate_protected_js_asset("runtime", runtime);
    quickjs_engine_module = harden_release_js_asset(std::move(quickjs_engine_module));
    quickjs_engine_module = ast_harden_release_js("engine", quickjs_engine_module, poly.seed_commitment);
    validate_protected_js_asset("quickjs-engine", quickjs_engine_module);
  }
  if (!production_hardening) {
    // Development output remains readable and uses the descriptive ABI names.
  }
  const auto runtime_file_name = named_output(hardened_release_asset ? "" : (wasm_runtime ? "runtime-js-bridge" : "runtime"), ".js", runtime, options.hashed_assets);
  const auto runtime_name = hardened_release_asset ? "javascript/" + runtime_file_name : runtime_file_name;
  const auto quickjs_engine_file_name = named_output(hardened_release_asset ? "" : "quickjs-engine", ".js", quickjs_engine_module, options.hashed_assets);
  const auto quickjs_engine_name = hardened_release_asset ? "javascript/" + quickjs_engine_file_name : quickjs_engine_file_name;
  const auto quickjs_wasm_file_name = named_output(hardened_release_asset ? "" : "quickjs-runtime", ".wasm", quickjs_wasm_bytes, options.hashed_assets);
  const auto quickjs_wasm_name = hardened_release_asset ? "wasm/" + quickjs_wasm_file_name : quickjs_wasm_file_name;
  if (production_hardening) {
    worker_runtime = harden_release_js_asset(std::move(worker_runtime));
    worker_runtime = ast_harden_release_js("worker", worker_runtime, poly.seed_commitment);
    validate_protected_js_asset("worker", worker_runtime);
  }
  const auto worker_file_name = hardened_release_asset ? named_output("", ".js", worker_runtime, options.hashed_assets) : std::string{};
  const auto worker_name = hardened_release_asset ? "javascript/" + worker_file_name : std::string{};
  trace.info("Generated protected worker with " + std::to_string(lazy_registry_chunks.size()) + " lazy registry chunks and hardened runtime assets");
  trace.phase("Build package metadata and lazy route sections");
  auto section_plan = package_sections::make_plan({
      options, profile, graph, compiled_routes, js_bridge, remote_vendor_options,
      vendor_lock_path, vendor_lock_digest, vendor_lock_mode, release_policy, assets, poly,
      runtime_name, runtime, wasm_name, wasm_bytes, style_name, css,
      quickjs_engine_name, quickjs_engine_module, quickjs_wasm_name, quickjs_wasm_bytes,
      worker_name, worker_runtime, qjs_probe, js_preview, wasm_runtime,
      hardened_release_asset, production_hardening, external_lazy_registry});
  auto package_plan = std::move(section_plan.plan);
  const auto& package_binding_token = section_plan.package_binding_token;
  const auto package_flags = section_plan.package_flags;
  trace.info("Prepared " + std::to_string(package_plan.size()) + " package sections before encryption and compression");
  trace.phase("Encrypt, compress, and assemble VBC package sections");
  package_plan::WriterOptions writer_options;
  writer_options.package_flags = package_flags;
  writer_options.compression_enabled = profile.compress_sections;
  writer_options.section_encryption_enabled = profile.crypto_provider_ready && true;
  writer_options.section_name_redaction_enabled = profile.strip_debug_metadata && true;
  writer_options.layout_polymorphism_enabled = profile.polymorphic && true;
  writer_options.layout_seed = poly.seed;
  writer_options.layout_max_padding = section_plan.layout_max_padding;
  writer_options.crypto_provider = selected_writer_crypto_provider(options.crypto_provider);

  if (hardened_release_asset) {
    std::filesystem::create_directories(assets_dir / "app");
    std::filesystem::create_directories(assets_dir / "javascript");
    std::filesystem::create_directories(assets_dir / "wasm");
    std::filesystem::create_directories(assets_dir / "images");
  }
  const auto package_temp_path = hardened_release_asset ? (assets_dir / "app" / ".package.tmp") : (assets_dir / ".app.vbc.tmp");
  trace.info("Encrypted and compressed " + std::to_string(package_plan.size()) + " VBC sections");
  trace.phase("Write and verify polymorphic package container");
  auto package_artifact = package_plan.write_verified(package_temp_path, writer_options);
  const auto& package_bytes = package_artifact.bytes;
  const auto& package_probe = package_artifact.package;
  trace.info("Wrote and structurally verified a " + std::to_string(package_bytes.size()) + " byte package with " + std::to_string(package_probe.sections.size()) + " sections");
  const auto package_file_name = named_output(hardened_release_asset ? "" : "app", ".vbc", package_bytes, options.hashed_assets);
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
    trace.phase("Harden and validate bootstrap loader");
    loader = harden_release_js_asset(std::move(loader));
    loader = ast_harden_release_js("loader", loader, poly.seed_commitment);
    validate_protected_js_asset("loader", loader);
    trace.info("Hardened bootstrap loader and validated release-safe JavaScript output");
  }
  const auto loader_file_name = named_output(hardened_release_asset ? "" : "loader", ".js", loader, options.hashed_assets);
  const auto loader_name = hardened_release_asset ? "javascript/" + loader_file_name : loader_file_name;
  std::vector<HtmlPreloadHint> startup_preloads;
  startup_preloads.push_back({"assets/" + loader_name, "modulepreload", "script", "text/javascript", true});
  startup_preloads.push_back({"assets/" + runtime_name, "modulepreload", "script", "text/javascript", true});
  startup_preloads.push_back({"assets/" + package_name, "preload", "fetch", "application/octet-stream", true});
  if (!quickjs_wasm_name.empty()) startup_preloads.push_back({"assets/" + quickjs_wasm_name, "preload", "fetch", "application/wasm", true});
  if (!worker_name.empty()) startup_preloads.push_back({"assets/" + worker_name, "modulepreload", "script", "text/javascript", true});
  const auto html = make_bootstrap_html(graph,
                                        "assets/" + loader_name,
                                        "assets/" + style_name,
                                        venom::package::sha256_sri(bytes_from_string(loader)),
                                        venom::package::sha256_sri(bytes_from_string(css)),
                                        startup_preloads);

  trace.phase("Emit distribution assets and integrity-bound bootstrap");
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
  if (external_lazy_registry) {
    for (std::size_t i = 0; i < js_bridge.bridge_registry_chunks.size(); ++i) {
      const auto& compiled_chunk = js_bridge.bridge_registry_chunks[i];
      const auto& runtime_chunk = lazy_registry_chunks[i];
      const auto relative = runtime_chunk.asset.substr(3u); // remove ../ prefix used by worker
      std::ofstream lazy_out(assets_dir / relative, std::ios::binary);
      if (!lazy_out) raise_error("VENOM-E5000", "failed to write lazy protected registry chunk: " + compiled_chunk.id);
      lazy_out.write(reinterpret_cast<const char*>(compiled_chunk.bytecode.data()),
                     static_cast<std::streamsize>(compiled_chunk.bytecode.size()));
      lazy_out.flush();
      if (!lazy_out) raise_error("VENOM-E5000", "failed to finalize lazy protected registry chunk: " + compiled_chunk.id);
    }
  }
  {
    std::ofstream qjs_wasm_out(assets_dir / quickjs_wasm_name, std::ios::binary);
    if (!qjs_wasm_out) {
      raise_error("VENOM-E5000", "failed to write " + (assets_dir / quickjs_wasm_name).string());
    }
    qjs_wasm_out.write(reinterpret_cast<const char*>(quickjs_wasm_bytes.data()), static_cast<std::streamsize>(quickjs_wasm_bytes.size()));
    qjs_wasm_out.flush();
    if (!qjs_wasm_out) {
      raise_error("VENOM-E5000", "failed to finish writing " + (assets_dir / quickjs_wasm_name).string());
    }
  }
  {
    const auto written_quickjs_wasm = read_bytes(assets_dir / quickjs_wasm_name);
    if (written_quickjs_wasm != quickjs_wasm_bytes) {
      raise_error("VENOM-E5000", "QuickJS WASM write verification failed");
    }
    const auto written_digest = venom::package::sha256_hex(written_quickjs_wasm);
    const auto expected_digest = venom::package::sha256_hex(quickjs_wasm_bytes);
    if (written_digest != expected_digest) {
      raise_error("VENOM-E5000", "QuickJS WASM digest changed after package binding generation");
    }
  }
  if (wasm_runtime) {
    std::ofstream wasm_out(assets_dir / wasm_name, std::ios::binary);
    if (!wasm_out) {
      raise_error("VENOM-E5000", "failed to write " + (assets_dir / wasm_name).string());
    }
    wasm_out.write(reinterpret_cast<const char*>(wasm_bytes.data()), static_cast<std::streamsize>(wasm_bytes.size()));
  }
  if (should_emit_external_asset_manifest(profile, options)) {
    write_text(assets_dir / "asset-manifest.txt", assets.manifest_text);
  }
  emit_public_assets(assets, assets_dir);
  if (chrome_extension_target) {
    chrome_extension::emit_extension_files(graph, options.output);
    trace.info("Emitted Manifest V3 extension files with stable chrome.scripting paths");
  }
  trace.info("Emitted " + std::to_string(assets.records.size()) + " public assets plus hashed runtime and package artifacts");

  trace.phase("Reopen output and verify package, hashes, and runtime artifacts");
  const auto validated_package = venom::package::read_package(package_path);
  trace.info("Reopened the distribution and verified package, asset, and runtime integrity metadata");
  if (!vendor_lock.present || options.refresh_vendors) {
    try {
      write_remote_vendor_lock(vendor_lock_path, js_bridge.remote_vendors);
    } catch (...) {
      std::error_code cleanup_error;
      std::filesystem::remove_all(options.output, cleanup_error);
      throw;
    }
  }
  trace.phase("Write provenance metadata and private build reports");
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
  const auto private_report_dir = std::filesystem::absolute(options.output).parent_path() /
      ".venom" / options.output.filename();
  const auto build_metadata_ref = hardened_release_asset ? std::string{} : std::string{"assets/build.json"};
  if (hardened_release_asset) {
    write_text(private_report_dir / "build.json", provenance.str());
  } else {
    write_text(options.output / build_metadata_ref, provenance.str());
  }
  write_text(private_report_dir / "protection-intents.json", js_bridge.protection_intent_ledger_json);
  if (!js_bridge.protected_api_typescript.empty())
    write_text(private_report_dir / "api" / "venom-exports.d.ts", js_bridge.protected_api_typescript);
  if (!js_bridge.protected_api_client_javascript.empty())
    write_text(private_report_dir / "api" / "venom-client.js", js_bridge.protected_api_client_javascript);
  if (!js_bridge.protected_api_client_typescript.empty())
    write_text(private_report_dir / "api" / "venom-client.d.ts", js_bridge.protected_api_client_typescript);
  if (!js_bridge.protected_contracts_json.empty())
    write_text(private_report_dir / "api" / "protected-contracts.json", js_bridge.protected_contracts_json);
  if (false) {
    const auto report_dir = options.output / "build" / "reports";
    write_text(report_dir / "execution-plan.txt", js_bridge.execution_plan_text);
    write_text(report_dir / "execution-plan.json", js_bridge.execution_plan_json);
    write_text(report_dir / "function-plan.txt", js_bridge.function_plan_text);
    write_text(report_dir / "function-plan.json", js_bridge.function_plan_json);
    write_text(report_dir / "function-extraction-plan.txt", js_bridge.extraction_plan_text);
    write_text(report_dir / "function-extraction-plan.json", js_bridge.extraction_plan_json);
    write_text(report_dir / "runtime-bridge-contract.json", js_bridge.runtime_bridge_contract_json);
    write_text(report_dir / "bridge-rewrite-plan.json", js_bridge.bridge_rewrite_report_json);
    if (!js_bridge.protected_api_typescript.empty())
      write_text(options.output / "assets" / "app" / "venom-exports.d.ts", js_bridge.protected_api_typescript);
    if (!js_bridge.protected_api_client_javascript.empty())
      write_text(options.output / "assets" / "app" / "venom-client.js", js_bridge.protected_api_client_javascript);
    if (!js_bridge.protected_api_client_typescript.empty())
      write_text(options.output / "assets" / "app" / "venom-client.d.ts", js_bridge.protected_api_client_typescript);
  }

  trace.info("Wrote reproducibility metadata, provenance records, and private build diagnostics");
  trace.phase("Finalize protection report");
  if (options.format != OutputFormat::Json && options.verbosity > 1) {
    print_build_protection_report(profile, options, package_path, package_bytes, validated_package, should_emit_external_asset_manifest(profile, options));
  }
  trace.info("Protection report passed for " + project_name + "; output is hardened, sealed, and fail-closed");

  const auto ts_cache = typescript::cache_stats();
  const auto qjs_cache = venom::quickjs::bytecode_cache_stats();
  const auto hardener_cache = hardener_cache_stats();
  const auto hardener_runtime = native_js_hardener::runtime_stats();
  std::ostringstream cache_report;
  cache_report << "{\"enabled\":" << (options.cache_enabled ? "true" : "false")
               << ",\"typescript\":{\"hits\":" << ts_cache.hits << ",\"misses\":" << ts_cache.misses << ",\"writes\":" << ts_cache.writes << "}"
               << ",\"quickjs_bytecode\":{\"hits\":" << qjs_cache.hits << ",\"misses\":" << qjs_cache.misses << ",\"writes\":" << qjs_cache.writes << "}"
               << ",\"javascript_hardener\":{\"hits\":" << hardener_cache.hits << ",\"misses\":" << hardener_cache.misses << ",\"writes\":" << hardener_cache.writes << "}}";
  const auto performance_report_path = private_report_dir / "build-performance.json";
  trace.write_performance_report(performance_report_path, cache_report.str());

  if (options.format == OutputFormat::Json) {
    std::cout << "{\"ok\":true,\"version\":\"" << VENOM_VERSION_STRING
              << "\",\"output\":\"" << json_escape(options.output.generic_string())
              << "\",\"package\":\"assets/" << json_escape(package_name)
              << "\",\"package_sha256\":\"" << venom::package::sha256_hex(package_bytes)
              << "\",\"build_metadata\":" << (build_metadata_ref.empty() ? "null" : "\"" + json_escape(build_metadata_ref) + "\"") << "}\n";
    return true;
  }

  trace.success("Protected distribution compiled successfully");
  if (options.verbosity == 0) {
    std::cout << "venom: built protected distribution => " << options.output.string() << "\n";
    return true;
  }
  trace.info("Build summary");
  trace.summary("Project", std::to_string(graph.files.size()) + " files, " + std::to_string(graph.routes.size()) + " route");
  trace.summary("Protected", std::to_string(js_bridge.chunks.size()) + " chunks, " + std::to_string(js_bridge.protected_exports.size()) + " export");
  trace.summary("Package", std::to_string(validated_package.file_size) + " bytes, " + std::to_string(validated_package.sections.size()) + " sections");
  trace.summary("Security", std::string("hardened, sealed, fail-closed; host fallback ") + (release_policy.fallback_allowed ? "allowed" : "denied"));
  trace.summary("Cache", "TS " + std::to_string(ts_cache.hits) + " hit/" + std::to_string(ts_cache.misses) + " miss; QuickJS " + std::to_string(qjs_cache.hits) + " hit/" + std::to_string(qjs_cache.misses) + " miss; hardener " + std::to_string(hardener_cache.hits) + " hit/" + std::to_string(hardener_cache.misses) + " miss");
  trace.summary("Hardener", std::to_string(hardener_runtime.calls) + " calls / " + std::to_string(hardener_runtime.initializations) + " runtime init / " + std::to_string(static_cast<long long>(hardener_runtime.execution_ms)) + " ms execution");
  trace.artifact("Output", options.output);
  if (options.verbosity > 1) {
    trace.summary("Runtime", options.runtime + " / " + release_policy.backend);
    trace.summary("Vendor lock", vendor_lock_mode + " / " + vendor_lock_digest.substr(0, 16) + "...");
    trace.summary("QuickJS probe", qjs_probe);
    trace.artifact("Package", options.output / "assets" / package_name);
    trace.artifact("Private reports", private_report_dir);
    trace.artifact("Performance", performance_report_path);
  }
  return true;
}

} // namespace venom::compiler

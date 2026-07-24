#include "pipeline/compatibility.hpp"
#include "pipeline/capability_analysis.hpp"
#include "core/version.hpp"
#include "generated/contracts/product_contracts.hpp"

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace venom::compiler {
namespace {
struct Finding {
  std::filesystem::path file;
  std::size_t line = 0;
  std::size_t column = 0;
  std::string feature;
  std::string status;
  std::string guidance;
};

std::string escape(const std::string& s) {
  std::string o;
  for (const char raw_c : s) {
    const auto c = static_cast<unsigned char>(raw_c);
    if (c == '"' || c == '\\') o += '\\';
    if (c == '\n') o += "\\n";
    else if (c == '\r') o += "\\r";
    else if (c == '\t') o += "\\t";
    else o += static_cast<char>(c);
  }
  return o;
}

void add_finding(std::vector<Finding>& out, const CapabilityOccurrence& x,
                 std::string feature, std::string status, std::string guidance) {
  out.push_back({x.file, x.line, x.column, std::move(feature), std::move(status), std::move(guidance)});
}

std::vector<Finding> evaluate(const CapabilityGraph& graph) {
  std::vector<Finding> findings;
  for (const auto& x : graph.occurrences) {
    if (x.kind == "syntax" && x.name == "dynamic_eval") {
      add_finding(findings, x, "javascript.eval", "denied", "Replace dynamic source evaluation with static modules.");
    } else if (x.kind == "syntax" && x.name == "function_constructor") {
      add_finding(findings, x, "javascript.function-constructor", "denied", "Precompile the function body or use a static dispatch table.");
    } else if (x.kind == "node" && x.name == "require") {
      add_finding(findings, x, "node.require", "unsupported", "Bundle browser dependencies before compiling with Venom.");
    } else if (x.kind == "node" && x.name == "umd_commonjs_branch") {
      add_finding(findings, x, "javascript.umd-commonjs-branch", "partial", "A guarded UMD CommonJS branch was detected; validate the browser-global branch in the real-browser matrix.");
    } else if (x.kind == "node" && x.name == "process") {
      add_finding(findings, x, "node.process", "unsupported", "Node.js globals are not available in the browser runtime.");
    } else if (x.kind == "browser" && x.name == "WebSocket") {
      add_finding(findings, x, "web.websocket", "unsupported", "Use fetch/polling or add a reviewed WebSocket host capability.");
    } else if (x.kind == "browser" && x.name == "ResizeObserver") {
      add_finding(findings, x, "dom.resize-observer", "partial", "Validate observer timing against the target browser matrix.");
    } else if (x.kind == "browser" && x.name == "MutationObserver") {
      add_finding(findings, x, "dom.mutation-observer", "partial", "Validate observer timing and mutation delivery ordering.");
    } else if (x.kind == "browser" && x.name == "IntersectionObserver") {
      add_finding(findings, x, "dom.intersection-observer", "partial", "Validate visibility and scheduling semantics in each target browser.");
    } else if (x.kind == "browser" && x.name == "SharedArrayBuffer") {
      add_finding(findings, x, "web.shared-array-buffer", "unsupported", "Requires cross-origin isolation and a dedicated reviewed bridge.");
    } else if (x.kind == "browser" && (x.name == "RTCPeerConnection" || x.name == "MediaDevices")) {
      add_finding(findings, x, "web.webrtc", "unsupported", "Keep real-time media outside the protected runtime until a reviewed capability module exists.");
    } else if (x.kind == "browser" && x.name == "WebGPU") {
      add_finding(findings, x, "web.webgpu", "unsupported", "WebGPU is not currently exposed by the host bridge.");
    } else if (x.kind == "browser" && (x.name == "Worker" || x.name == "BroadcastChannel" || x.name == "EventSource")) {
      add_finding(findings, x, "web." + std::string(x.name == "Worker" ? "worker" : x.name == "BroadcastChannel" ? "broadcast-channel" : "event-source"),
                  "partial", "This API requires fixture-specific browser validation and lifecycle testing.");
    } else if (x.kind == "module" && x.name == "dynamic_import") {
      add_finding(findings, x, "javascript.dynamic-import", "partial", "Ensure every dynamic chunk is statically discoverable, vendored, and covered by browser validation.");
    }
  }
  std::sort(findings.begin(), findings.end(), [](const Finding& a, const Finding& b) {
    if (a.file != b.file) return a.file.generic_string() < b.file.generic_string();
    if (a.line != b.line) return a.line < b.line;
    if (a.column != b.column) return a.column < b.column;
    return a.feature < b.feature;
  });
  findings.erase(std::unique(findings.begin(), findings.end(), [](const Finding& a, const Finding& b) {
    return a.file == b.file && a.line == b.line && a.column == b.column && a.feature == b.feature;
  }), findings.end());
  return findings;
}
}

bool run_compatibility_check(const CompatibilityOptions& options) {
  const auto graph = analyze_capabilities(options.input);
  const auto findings = evaluate(graph);
  const bool compatible = std::none_of(findings.begin(), findings.end(), [](const Finding& f) {
    return f.status == "denied" || f.status == "unsupported";
  });
  if (options.format == OutputFormat::Json) {
    std::cout << "{\"schema_version\":2,\"analysis\":\"syntax-aware-capability-graph-v1\",\"compatible\":"
              << (compatible ? "true" : "false") << ",\"findings\":[";
    for (std::size_t i = 0; i < findings.size(); ++i) {
      if (i) std::cout << ',';
      const auto& f = findings[i];
      std::cout << "{\"file\":\"" << escape(f.file.generic_string()) << "\",\"line\":" << f.line
                << ",\"column\":" << f.column << ",\"feature\":\"" << f.feature
                << "\",\"status\":\"" << f.status << "\",\"guidance\":\"" << escape(f.guidance) << "\"}";
    }
    std::cout << "],\"capability_graph\":" << capability_graph_json(graph) << "}\n";
  } else {
    std::cout << "Venom compatibility check: " << options.input.string() << "\n"
              << "Analyzer: syntax-aware-capability-graph-v1\n\n";
    if (findings.empty()) std::cout << "[PASS] No known incompatible syntax or capability use detected.\n";
    for (const auto& f : findings) {
      std::cout << '[' << (f.status == "partial" ? "WARN" : "FAIL") << "] " << f.file.string() << ':' << f.line << ':' << f.column
                << ' ' << f.feature << " (" << f.status << ")\n       " << f.guidance << "\n";
    }
    std::cout << "\nResult: " << (compatible ? "compatible with declared warnings" : "incompatible capabilities found") << "\n";
  }
  return compatible;
}

void run_capability_analysis(const CompatibilityOptions& options) {
  const auto graph = analyze_capabilities(options.input);
  if (options.format == OutputFormat::Json) std::cout << capability_graph_json(graph) << '\n';
  else print_capability_graph_text(graph);
}

void print_contracts(OutputFormat format) {
  if (format == OutputFormat::Json) {
    std::cout << "{\"product_version\":\"" << VENOM_VERSION_STRING
      << "\",\"package_format_version\":" << venom::contracts::kPackageFormatVersion
      << ",\"package_runtime_abi\":" << venom::contracts::kPackageRuntimeAbi
      << ",\"route_vm_contract\":\"" << venom::contracts::kRouteVmContract
      << "\",\"dom_command_contract\":\"" << venom::contracts::kDomCommandContract
      << "\",\"turbojs_module_bundle_contract\":\"" << venom::contracts::kTurboJsModuleBundleContract
      << "\",\"turbojs_runtime_abi\":" << venom::contracts::kTurboJsRuntimeAbi
      << ",\"host_bridge_abi\":" << venom::contracts::kHostBridgeAbi
      << ",\"configuration_schema\":" << venom::contracts::kConfigurationSchema
      << ",\"config_schema\":" << venom::contracts::kConfigurationSchema
      << ",\"lockfile_schema\":" << venom::contracts::kLockfileSchema << "}\n";
  } else {
    std::cout << "Venom product contracts\n\nProduct version: " << VENOM_VERSION_STRING
      << "\nPackage format version: " << venom::contracts::kPackageFormatVersion
      << "\nPackage runtime ABI: " << venom::contracts::kPackageRuntimeAbi
      << "\nRoute VM contract: " << venom::contracts::kRouteVmContract
      << "\nDOM command contract: " << venom::contracts::kDomCommandContract
      << "\nTurboJS module bundle contract: " << venom::contracts::kTurboJsModuleBundleContract
      << "\nTurboJS runtime ABI: " << venom::contracts::kTurboJsRuntimeAbi
      << "\nHost bridge ABI: " << venom::contracts::kHostBridgeAbi
      << "\nConfiguration schema: " << venom::contracts::kConfigurationSchema
      << "\nLockfile schema: " << venom::contracts::kLockfileSchema << "\n";
  }
}
} // namespace venom::compiler

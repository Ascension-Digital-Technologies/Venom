#include "function_dependencies.hpp"

#include "venom/frontends/javascript/frontend.hpp"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace venom::compiler {
namespace {

using Declaration = frontend::ScopeDeclaration;

std::string lower_ascii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

std::string mask_literals_and_comments(const std::string& source) {
  std::string out = source;
  bool single=false, dbl=false, templ=false, line=false, block=false, escape=false;
  bool regex=false, regex_class=false;
  auto regex_can_start = [&](std::size_t at) {
    std::size_t p = at;
    while (p > 0u && std::isspace(static_cast<unsigned char>(source[p - 1u]))) --p;
    if (p == 0u) return true;
    const char prev = source[p - 1u];
    return std::string_view("(=:[,!&|?{};").find(prev) != std::string_view::npos;
  };
  for (std::size_t i=0;i<out.size();++i) {
    const char c=out[i]; const char n=i+1u<out.size()?out[i+1u]:'\0';
    if (line) { if (c=='\n') line=false; else out[i]=' '; continue; }
    if (block) { out[i]=' '; if(c=='*'&&n=='/'){out[i+1u]=' ';++i;block=false;} continue; }
    if (regex) {
      out[i] = ' ';
      if (escape) { escape=false; continue; }
      if (c=='\\') { escape=true; continue; }
      if (c=='[') { regex_class=true; continue; }
      if (c==']') { regex_class=false; continue; }
      if (c=='/' && !regex_class) {
        regex=false;
        while (i+1u<out.size() && std::isalpha(static_cast<unsigned char>(out[i+1u]))) out[++i]=' ';
      }
      continue;
    }
    if (escape) { out[i]=' '; escape=false; continue; }
    if ((single||dbl||templ)&&c=='\\') { out[i]=' '; escape=true; continue; }
    if (single) { out[i]=' '; if(c=='\'') single=false; continue; }
    if (dbl) { out[i]=' '; if(c=='"') dbl=false; continue; }
    if (templ) { out[i]=' '; if(c=='`') templ=false; continue; }
    if(c=='/'&&n=='/'){out[i]=out[i+1u]=' ';++i;line=true;continue;}
    if(c=='/'&&n=='*'){out[i]=out[i+1u]=' ';++i;block=true;continue;}
    if(c=='/' && regex_can_start(i)){out[i]=' ';regex=true;regex_class=false;continue;}
    if(c=='\''){out[i]=' ';single=true;continue;}
    if(c=='"'){out[i]=' ';dbl=true;continue;}
    if(c=='`'){out[i]=' ';templ=true;continue;}
  }
  return out;
}

const std::unordered_set<std::string>& allowed_globals() {
  static const std::unordered_set<std::string> values = {
    "undefined","NaN","Infinity","Math","JSON","Number","String","Boolean","Array","Object",
    "BigInt","Date","RegExp","Map","Set","WeakMap","WeakSet","Promise","Error","TypeError",
    "RangeError","ReferenceError","SyntaxError","EvalError","URIError","AggregateError","parseInt",
    "parseFloat","isFinite","isNaN","encodeURI","decodeURI","encodeURIComponent","decodeURIComponent",
    "Uint8Array","Uint8ClampedArray","Uint16Array","Uint32Array","Int8Array","Int16Array","Int32Array",
    "Float32Array","Float64Array","BigInt64Array","BigUint64Array","ArrayBuffer","SharedArrayBuffer",
    "DataView","Atomics","Reflect","Proxy","Intl","console","globalThis"
  };
  return values;
}

const std::unordered_set<std::string>& browser_globals() {
  static const std::unordered_set<std::string> values = {
    "window", "document", "navigator", "location", "history", "localStorage", "sessionStorage",
    "fetch", "XMLHttpRequest", "WebSocket", "Worker", "SharedWorker", "ServiceWorker", "HTMLElement",
    "customElements", "requestAnimationFrame", "cancelAnimationFrame", "EventSource", "BroadcastChannel",
    "MutationObserver", "ResizeObserver", "IntersectionObserver"
  };
  return values;
}

std::unordered_map<std::string, Declaration> index_declarations(
    const frontend::FunctionScopeAnalysis& analysis) {
  std::unordered_map<std::string, Declaration> out;
  for (const auto& declaration : analysis.declarations) out.emplace(declaration.name, declaration);
  return out;
}

bool collect(const std::vector<std::string>& captures,
             const std::unordered_map<std::string,Declaration>& declarations,
             std::vector<LiftedFunctionDependency>& ordered,
             std::unordered_set<std::string>& resolved,
             std::unordered_set<std::string>& visiting,
             std::string& reason) {
  for (const auto& capture : captures) {
    if (allowed_globals().count(capture) || resolved.count(capture)) continue;
    if (browser_globals().count(capture)) {
      reason = "browser-only lexical capture requires an explicit capability: " + capture;
      return false;
    }
    const auto found = declarations.find(capture);
    if (found == declarations.end()) {
      reason = "unresolved lexical capture: " + capture;
      return false;
    }
    if (visiting.count(capture)) continue;
    visiting.insert(capture);
    const auto& dependency = found->second;
    if (dependency.kind == Declaration::Kind::MutableBinding) {
      reason = "mutable lexical capture is not safe to lift: " + capture;
      return false;
    }
    if (dependency.kind == Declaration::Kind::UnsupportedConstant) {
      reason = "constant capture is not a supported immutable literal: " + capture;
      return false;
    }
    if (dependency.kind == Declaration::Kind::ImportBinding) {
      reason = "imported lexical capture requires module dependency lowering: " + capture;
      return false;
    }
    if (dependency.kind == Declaration::Kind::Function) {
      if (!dependency.unsafe_features.empty()) {
        reason = "helper " + capture + " is runtime-bound: " + dependency.unsafe_features.front();
        return false;
      }
      if (!collect(dependency.free_identifiers, declarations, ordered, resolved, visiting, reason)) return false;
    }
    visiting.erase(capture);
    if (resolved.insert(capture).second)
      ordered.push_back({dependency.name, dependency.declaration});
  }
  return true;
}

} // namespace

FunctionDependencyResolution resolve_liftable_function_dependencies(
    const std::string& source, const std::string& target_declaration,
    const std::string& target_name, const std::string& target_params,
    const std::string& source_name, bool module) {
  (void)target_declaration;
  (void)target_params;
  FunctionDependencyResolution result;
  const auto analysis = frontend::analyze_function_scope(source, source_name, target_name, module);
  if (!analysis.target_found) {
    result.reason = "AST frontend could not locate protected function: " + target_name;
    return result;
  }
  if (!analysis.target_unsafe_features.empty()) {
    result.reason = "protected function is runtime-bound: " + analysis.target_unsafe_features.front();
    return result;
  }
  const auto declarations = index_declarations(analysis);
  std::unordered_set<std::string> resolved;
  std::unordered_set<std::string> visiting{target_name};
  result.success = collect(analysis.target_free_identifiers, declarations,
                           result.dependencies, resolved, visiting, result.reason);
  return result;
}

bool verify_isolated_protected_unit(const std::string& declaration, std::string& reason) {
  if (declaration.size() < 16384u) {
    reason = "isolated fallback is reserved for large self-contained compilation units";
    return false;
  }
  const auto masked = lower_ascii(mask_literals_and_comments(declaration));
  static const std::pair<std::string_view, std::string_view> unsafe[] = {
    {"document.", "document"}, {"window.", "window"}, {"globalthis.document", "document"},
    {"globalthis.window", "window"}, {"navigator.", "navigator"}, {"localstorage.", "localStorage"},
    {"sessionstorage.", "sessionStorage"}, {"eval(", "dynamic eval"}, {"new.target", "new.target binding"},
    {"import.meta", "module metadata"}
  };
  for (const auto& item : unsafe) {
    if (masked.find(item.first) != std::string::npos) {
      reason = "isolated unit captures browser-only or dynamic feature: " + std::string(item.second);
      return false;
    }
  }
  reason = "verified isolated protected compilation unit";
  return true;
}

} // namespace venom::compiler

#include "base/error.hpp"
#include "pipeline/planner.hpp"
#include "graph/module_graph.hpp"
#include "frontends/javascript/frontend.hpp"
#include "frontends/typescript/frontend.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace venom::compiler {
namespace {
struct FunctionRecommendation {
  std::string name;
  std::uint32_t line = 0;
  std::string runtime = "protected";
  int confidence = 70;
  int purity = 50;
  std::string source = "planner";
  std::vector<std::string> reasons;
  std::vector<std::string> import_captures;
};
struct ImportedBindingRecommendation {
  std::string specifier;
  std::string imported_name;
  bool namespace_import = false;
};
struct ReexportRecommendation {
  std::string specifier;
  std::string imported_name;
  bool namespace_export = false;
};
struct FileRecommendation {
  std::filesystem::path file;
  std::string runtime = "protected";
  int confidence = 70;
  std::string source = "planner";
  std::vector<std::string> reasons;
  std::vector<FunctionRecommendation> functions;
  std::vector<std::filesystem::path> module_dependencies;
  std::map<std::string, ImportedBindingRecommendation> imported_bindings;
  std::set<std::string> side_effect_imports;
  std::map<std::string, std::string> exported_symbol_runtimes;
  std::map<std::string, std::string> local_export_bindings;
  std::map<std::string, ReexportRecommendation> explicit_reexports;
  std::vector<std::string> export_all_specifiers;
  std::vector<std::string> unresolved_export_kinds;
  std::vector<std::string> module_features;
};
struct PlanResult {
  std::map<std::filesystem::path, FileRecommendation> files;
  std::size_t protected_files = 0;
  std::size_t browser_files = 0;
  std::size_t review_files = 0;
  std::size_t protected_functions = 0;
  std::size_t browser_functions = 0;
  std::size_t review_functions = 0;
  bool strict_pass = true;
};

std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
  return value;
}
std::string trim(std::string value) {
  const auto first=value.find_first_not_of(" \t\r\n");
  if(first==std::string::npos)return {};
  return value.substr(first,value.find_last_not_of(" \t\r\n")-first+1);
}
std::string esc(const std::string& s){
  std::ostringstream o;
  for (const char raw_c : s) { const auto c = static_cast<unsigned char>(raw_c);
    if(c=='"')o<<"\\\""; else if(c=='\\')o<<"\\\\"; else if(c=='\n')o<<"\\n";
    else if(c=='\r')o<<"\\r"; else if(c=='\t')o<<"\\t"; else if(c<0x20)o<<'?'; else o<<static_cast<char>(c);
  }
  return o.str();
}
bool is_js(const std::filesystem::path& p){
  const auto e=lower(p.extension().string());
  return e==".js"||e==".mjs"||e==".cjs"||e==".jsx"||e==".ts"||e==".tsx";
}
bool path_is_within(const std::filesystem::path& candidate,const std::filesystem::path& parent){
  if(parent.empty())return false;
  const auto child=std::filesystem::absolute(candidate).lexically_normal();
  const auto base=std::filesystem::absolute(parent).lexically_normal();
  auto child_it=child.begin();
  for(auto base_it=base.begin();base_it!=base.end();++base_it,++child_it)
    if(child_it==child.end()||*child_it!=*base_it)return false;
  return true;
}
bool pattern_match(const std::filesystem::path& path,const std::string& pattern){
  auto p=lower(path.generic_string()); auto q=lower(pattern); std::replace(q.begin(),q.end(),'\\','/');
  return !q.empty() && p.find(q)!=std::string::npos;
}
std::vector<std::string> parse_list(std::string value){
  value=trim(value); if(value.size()>=2&&value.front()=='['&&value.back()==']')value=value.substr(1,value.size()-2);
  std::vector<std::string> out; std::string current; bool quoted=false; char quote=0;
  for(char c:value){
    if((c=='\''||c=='"')){if(!quoted){quoted=true;quote=c;}else if(quote==c)quoted=false;continue;}
    if(c==','&&!quoted){current=trim(current);if(!current.empty())out.push_back(current);current.clear();} else current+=c;
  }
  current=trim(current);if(!current.empty())out.push_back(current);return out;
}
void load_config_rules(const std::filesystem::path& input, PlannerOptions& options){
  auto path=options.config_file;
  if(path.empty()){
    auto current=std::filesystem::absolute(input);
    if(!std::filesystem::is_directory(current))current=current.parent_path();
    for(;;){auto candidate=current/"venom.toml";if(std::filesystem::exists(candidate)){path=candidate;break;}if(current==current.root_path())break;current=current.parent_path();}
  }
  if(path.empty()||!std::filesystem::exists(path))return;
  std::ifstream in(path); std::string section,line;
  while(std::getline(in,line)){
    const auto hash=line.find('#');if(hash!=std::string::npos)line.erase(hash);line=trim(line);if(line.empty())continue;
    if(line.front()=='['&&line.back()==']'){section=trim(line.substr(1,line.size()-2));continue;}
    const auto eq=line.find('=');if(eq==std::string::npos)continue;
    const auto key=trim(line.substr(0,eq)),value=trim(line.substr(eq+1));
    if(section=="planner"&&(key=="protect"||key=="protected")){auto v=parse_list(value);options.config_protected_patterns.insert(options.config_protected_patterns.end(),v.begin(),v.end());}
    if(section=="planner"&&key=="browser"){auto v=parse_list(value);options.config_browser_patterns.insert(options.config_browser_patterns.end(),v.begin(),v.end());}
  }
  options.config_file=path;
}

std::string annotation_before(const std::string& source, std::size_t begin) {
  const auto window_begin = begin > 1024 ? begin - 1024 : 0;
  const auto prefix = lower(source.substr(window_begin, begin - window_begin));
  const auto browser = prefix.rfind("@venom: browser");
  const auto protect = prefix.rfind("@venom: protected");
  const auto marker = std::max(browser == std::string::npos ? 0 : browser + 1,
                               protect == std::string::npos ? 0 : protect + 1);
  if (marker == 0) return {};
  const auto after = prefix.substr(marker - 1);
  const auto newline = after.find('\n');
  if (newline != std::string::npos && after.substr(newline + 1).find_first_not_of(" \t\r\n") != std::string::npos) return {};
  return browser != std::string::npos && browser >= protect ? "browser" : "protected";
}

bool has_identifier(const std::vector<std::string>& identifiers, const std::vector<std::string>& candidates,
                    std::string* matched = nullptr) {
  for (const auto& identifier : identifiers) {
    if (std::find(candidates.begin(), candidates.end(), identifier) != candidates.end()) {
      if (matched) *matched = identifier;
      return true;
    }
  }
  return false;
}


std::filesystem::path resolve_module_dependency(const std::filesystem::path& from,
                                                const std::string& specifier,
                                                const std::map<std::filesystem::path, FileRecommendation>& files) {
  for (const auto& candidate_text : graph::module_resolution_candidates(from.generic_string(), specifier)) {
    const auto candidate = std::filesystem::path(candidate_text).lexically_normal();
    if (files.find(candidate) != files.end()) return candidate;
  }
  return {};
}

void propagate_module_runtimes(PlanResult& result) {
  std::map<std::filesystem::path, std::vector<std::filesystem::path>> graph;
  std::map<std::filesystem::path, std::map<std::string, std::filesystem::path>> resolved_by_specifier;

  const auto authoritative_file = [](const FileRecommendation& file) {
    return file.source == "annotation" || file.source == "config-rule" || file.source == "manual-cli";
  };
  const auto apply_function_runtime = [](FunctionRecommendation& fn, const std::string& runtime,
                                       const std::string& reason) {
    if (fn.source == "annotation") return;
    if (runtime == "browser") {
      fn.runtime = "browser";
      fn.confidence = std::max(fn.confidence, 98);
      fn.purity = std::min(fn.purity, 15);
      fn.reasons.push_back(reason);
    } else if (runtime == "manual-review" && fn.runtime != "browser") {
      fn.runtime = "manual-review";
      fn.confidence = std::max(fn.confidence, 96);
      fn.purity = std::min(fn.purity, 30);
      fn.reasons.push_back(reason);
    }
  };
  const auto refresh_export_runtimes = [](FileRecommendation& file) {
    for (const auto& export_binding : file.local_export_bindings) {
      const auto& exported_name = export_binding.first;
      const auto& local_name = export_binding.second;
      const auto function = std::find_if(file.functions.begin(), file.functions.end(), [&local_name](const auto& fn) {
        return fn.name == local_name;
      });
      if (function != file.functions.end()) file.exported_symbol_runtimes[exported_name] = function->runtime;
      else if (file.exported_symbol_runtimes.find(exported_name) == file.exported_symbol_runtimes.end())
        file.exported_symbol_runtimes[exported_name] = "manual-review";
    }
  };
  const auto apply_file_dependency = [&](FileRecommendation& file, const FileRecommendation& target,
                                         const std::filesystem::path& dependency) {
    if (authoritative_file(file)) return;
    if (target.runtime == "browser") {
      file.runtime = "browser";
      file.confidence = std::max(file.confidence, 98);
      file.reasons.push_back("imports browser-bound module: " + dependency.generic_string());
      for (auto& fn : file.functions) {
        apply_function_runtime(fn, "browser", "module dependency is browser-bound: " + dependency.generic_string());
      }
    } else if (target.runtime == "manual-review" && file.runtime != "browser") {
      file.runtime = "manual-review";
      file.confidence = std::max(file.confidence, 96);
      file.reasons.push_back("imported module requires review: " + dependency.generic_string());
      for (auto& fn : file.functions) {
        apply_function_runtime(fn, "manual-review", "module dependency closure requires review: " + dependency.generic_string());
      }
    }
  };

  for (auto& [path, file] : result.files) {
    for (const auto& unresolved : file.module_dependencies) {
      const auto specifier = unresolved.generic_string();
      const auto resolved = resolve_module_dependency(path, specifier, result.files);
      if (!resolved.empty()) {
        graph[path].push_back(resolved);
        resolved_by_specifier[path][specifier] = resolved;
        continue;
      }
      if (authoritative_file(file)) continue;

      bool affects_file = file.side_effect_imports.count(specifier) != 0;
      bool affected_function = false;
      for (auto& fn : file.functions) {
        for (const auto& capture : fn.import_captures) {
          const auto binding = file.imported_bindings.find(capture);
          if (binding == file.imported_bindings.end() || binding->second.specifier != specifier) continue;
          affected_function = true;
          apply_function_runtime(fn, "manual-review", "module dependency is not locally closed: " + specifier);
        }
      }
      // Dynamic imports and other references without a static binding affect the module as a whole.
      if (!affected_function) affects_file = true;
      if (affects_file) {
        file.runtime = "manual-review";
        file.confidence = std::max(file.confidence, 96);
        file.reasons.push_back((!specifier.empty() && specifier[0] == '.' ?
            "unresolved relative module dependency: " : "external module dependency requires policy review: ") + specifier);
        for (auto& fn : file.functions) {
          apply_function_runtime(fn, "manual-review", "module dependency is not locally closed: " + specifier);
        }
      }
    }
  }

  std::map<std::filesystem::path, int> state;
  std::set<std::filesystem::path> cycle_members;
  std::function<void(const std::filesystem::path&, std::vector<std::filesystem::path>&)> visit =
      [&](const std::filesystem::path& path, std::vector<std::filesystem::path>& stack) {
    if (state[path] == 2) return;
    if (state[path] == 1) {
      const auto found = std::find(stack.begin(), stack.end(), path);
      if (found != stack.end()) cycle_members.insert(found, stack.end());
      return;
    }
    state[path] = 1;
    stack.push_back(path);
    for (const auto& dependency : graph[path]) {
      visit(dependency, stack);
      refresh_export_runtimes(result.files[dependency]);
    }

    auto& current = result.files[path];
    refresh_export_runtimes(current);

    // Resolve explicit and star re-exports after their source modules have been visited.
    for (const auto& [exported_name, reexport] : current.explicit_reexports) {
      const auto resolved_it = resolved_by_specifier[path].find(reexport.specifier);
      if (resolved_it == resolved_by_specifier[path].end()) {
        current.exported_symbol_runtimes[exported_name] = "manual-review";
        current.reasons.push_back("re-export dependency is not locally closed: " + reexport.specifier);
        continue;
      }
      const auto& dependency_path = resolved_it->second;
      const auto& target = result.files[dependency_path];
      if (reexport.namespace_export) {
        current.exported_symbol_runtimes[exported_name] = target.runtime;
        current.reasons.push_back("namespace re-export uses whole-module runtime: " + exported_name + " from " + dependency_path.generic_string());
        continue;
      }
      const auto target_symbol = target.exported_symbol_runtimes.find(reexport.imported_name);
      if (target_symbol == target.exported_symbol_runtimes.end()) {
        current.exported_symbol_runtimes[exported_name] = "manual-review";
        current.reasons.push_back("re-exported symbol is not statically resolved: " + reexport.imported_name + " from " + dependency_path.generic_string());
      } else {
        current.exported_symbol_runtimes[exported_name] = target_symbol->second;
        current.reasons.push_back("resolved re-export: " + exported_name + " <- " + reexport.imported_name + " from " + dependency_path.generic_string());
      }
    }
    for (const auto& specifier : current.export_all_specifiers) {
      const auto resolved_it = resolved_by_specifier[path].find(specifier);
      if (resolved_it == resolved_by_specifier[path].end()) {
        current.reasons.push_back("export-star dependency is not locally closed: " + specifier);
        continue;
      }
      const auto& dependency_path = resolved_it->second;
      const auto& target = result.files[dependency_path];
      for (const auto& [name, runtime] : target.exported_symbol_runtimes) {
        if (name == "default") continue;
        const auto existing = current.exported_symbol_runtimes.find(name);
        if (existing == current.exported_symbol_runtimes.end()) {
          current.exported_symbol_runtimes[name] = runtime;
        } else if (existing->second != runtime) {
          existing->second = "manual-review";
          current.reasons.push_back("ambiguous export-star runtime for symbol: " + name);
        }
      }
      current.reasons.push_back("resolved export-star dependency: " + dependency_path.generic_string());
    }

    if (!authoritative_file(current)) {
      // Side-effect imports and namespace imports retain whole-module semantics.
      for (const auto& specifier : current.side_effect_imports) {
        const auto resolved = resolved_by_specifier[path].find(specifier);
        if (resolved != resolved_by_specifier[path].end()) {
          apply_file_dependency(current, result.files[resolved->second], resolved->second);
        }
      }

      for (auto& fn : current.functions) {
        for (const auto& capture : fn.import_captures) {
          const auto binding_it = current.imported_bindings.find(capture);
          if (binding_it == current.imported_bindings.end()) continue;
          const auto& binding = binding_it->second;
          const auto resolved_it = resolved_by_specifier[path].find(binding.specifier);
          if (resolved_it == resolved_by_specifier[path].end()) continue;
          const auto& dependency_path = resolved_it->second;
          const auto& target = result.files[dependency_path];

          if (binding.namespace_import) {
            apply_function_runtime(fn, target.runtime,
                target.runtime == "browser" ? "namespace import reaches browser-bound module: " + dependency_path.generic_string() :
                "namespace import requires module review: " + dependency_path.generic_string());
            continue;
          }

          const auto exported = target.exported_symbol_runtimes.find(binding.imported_name);
          if (exported == target.exported_symbol_runtimes.end()) {
            apply_function_runtime(fn, "manual-review",
                "imported symbol is not statically resolved: " + binding.imported_name + " from " + dependency_path.generic_string());
            continue;
          }
          if (exported->second == "browser") {
            apply_function_runtime(fn, "browser",
                "imports browser-bound symbol: " + binding.imported_name + " from " + dependency_path.generic_string());
          } else if (exported->second == "manual-review") {
            apply_function_runtime(fn, "manual-review",
                "imported symbol requires review: " + binding.imported_name + " from " + dependency_path.generic_string());
          } else {
            fn.reasons.push_back("resolved protected import: " + binding.imported_name + " from " + dependency_path.generic_string());
          }
        }
      }

      const auto browser_count = std::count_if(current.functions.begin(), current.functions.end(),
          [](const auto& fn){ return fn.runtime == "browser"; });
      const auto review_count = std::count_if(current.functions.begin(), current.functions.end(),
          [](const auto& fn){ return fn.runtime == "manual-review"; });
      const auto browser_export = std::any_of(current.exported_symbol_runtimes.begin(), current.exported_symbol_runtimes.end(),
          [](const auto& item){ return item.second == "browser"; });
      const auto review_export = std::any_of(current.exported_symbol_runtimes.begin(), current.exported_symbol_runtimes.end(),
          [](const auto& item){ return item.second == "manual-review"; });
      if (review_count > 0 || review_export) {
        current.runtime = "manual-review";
        current.confidence = std::max(current.confidence, 96);
      } else if (browser_count > 0 || browser_export) {
        current.runtime = "browser";
        current.confidence = std::max(current.confidence, 94);
      } else if (!current.functions.empty() || !current.exported_symbol_runtimes.empty()) {
        current.runtime = "protected";
      }
    }
    refresh_export_runtimes(current);
    stack.pop_back();
    state[path] = 2;
  };
  for (const auto& [path, _] : result.files) {
    std::vector<std::filesystem::path> stack;
    visit(path, stack);
  }
  for (const auto& path : cycle_members) result.files[path].reasons.push_back("module dependency cycle validated");
}

std::vector<FunctionRecommendation> analyze_functions(const std::string& original_source,
                                                         const std::string& filename) {
  std::string source = original_source;
  if (lower(std::filesystem::path(filename).extension().string()) == ".ts" ||
      lower(std::filesystem::path(filename).extension().string()) == ".tsx") {
    source = typescript::transpile(source, filename).javascript;
  }

  frontend::ParseSummary parsed;
  bool module = true;
  try {
    parsed = frontend::parse(source, filename, true);
  } catch (...) {
    module = false;
    parsed = frontend::parse(source, filename, false);
  }

  static const std::vector<std::string> browser_globals = {
    "window", "document", "navigator", "location", "history", "localStorage", "sessionStorage",
    "HTMLElement", "Element", "Node", "MutationObserver", "ResizeObserver", "IntersectionObserver",
    "requestAnimationFrame", "cancelAnimationFrame", "XMLHttpRequest", "WebSocket", "Worker",
    "SharedWorker", "EventSource", "customElements", "screen", "fetch", "alert", "confirm", "prompt"
  };
  static const std::vector<std::string> nondeterministic = {
    "Date", "performance", "crypto", "setTimeout", "setInterval", "queueMicrotask", "console"
  };

  std::vector<FunctionRecommendation> result;
  std::vector<std::vector<std::string>> dependency_calls;
  result.reserve(parsed.top_level_functions.size());
  dependency_calls.reserve(parsed.top_level_functions.size());

  for (const auto& function : parsed.top_level_functions) {
    FunctionRecommendation recommendation;
    recommendation.name = function.name;
    recommendation.line = static_cast<std::uint32_t>(function.line);
    const auto annotation = annotation_before(source, function.begin);
    if (!annotation.empty()) {
      recommendation.runtime = annotation;
      recommendation.confidence = 100;
      recommendation.purity = annotation == "protected" ? 90 : 20;
      recommendation.source = "annotation";
      recommendation.reasons = {"explicit @venom function annotation"};
      dependency_calls.emplace_back();
      result.push_back(std::move(recommendation));
      continue;
    }

    const auto scope = frontend::analyze_function_scope(source, filename, function.name, module);
    dependency_calls.push_back(scope.target_direct_calls);
    recommendation.import_captures = scope.target_import_captures;
    std::string matched;
    if (!scope.target_unsafe_features.empty()) {
      recommendation.runtime = "manual-review";
      recommendation.confidence = 100;
      recommendation.purity = 10;
      recommendation.reasons.push_back("unsupported AST semantic: " + scope.target_unsafe_features.front());
    } else if (!scope.target_indirect_calls.empty()) {
      recommendation.runtime = "manual-review";
      recommendation.confidence = 97;
      recommendation.purity = 20;
      recommendation.reasons.push_back("indirect call target requires identity review: " + scope.target_indirect_calls.front());
    } else if (!scope.target_host_references.empty()) {
      recommendation.runtime = "browser";
      recommendation.confidence = 99;
      recommendation.purity = 10;
      recommendation.reasons.push_back("browser host reference in AST: " + scope.target_host_references.front());
    } else if (has_identifier(scope.target_free_identifiers, browser_globals, &matched)) {
      recommendation.runtime = "browser";
      recommendation.confidence = 98;
      recommendation.purity = 15;
      recommendation.reasons.push_back("browser global captured by AST scope: " + matched);
    } else if (!scope.target_mutable_captures.empty()) {
      recommendation.runtime = "manual-review";
      recommendation.confidence = 97;
      recommendation.purity = 25;
      recommendation.reasons.push_back("reads mutable captured binding: " + scope.target_mutable_captures.front());
    } else if (!scope.target_class_captures.empty()) {
      recommendation.runtime = "manual-review";
      recommendation.confidence = 98;
      recommendation.purity = 20;
      recommendation.reasons.push_back("captures class or constructor requiring identity review: " + scope.target_class_captures.front());
    } else if (!scope.target_unsupported_captures.empty()) {
      recommendation.runtime = "manual-review";
      recommendation.confidence = 96;
      recommendation.purity = 30;
      recommendation.reasons.push_back("captures non-primitive constant requiring serialization review: " + scope.target_unsupported_captures.front());
    } else {
      int purity = 78;
      if (function.generator) {
        purity -= 20;
        recommendation.reasons.push_back("generator lifecycle requires review");
      }
      if (has_identifier(scope.target_free_identifiers, nondeterministic, &matched)) {
        purity -= 25;
        recommendation.reasons.push_back("observable or nondeterministic global: " + matched);
      }
      if (!scope.target_effects.empty()) {
        purity -= 35;
        recommendation.reasons.push_back("observable closure effect: " + scope.target_effects.front());
      }
      if (scope.target_free_identifiers.empty()) {
        purity += 10;
        recommendation.reasons.push_back("no free identifiers in AST scope");
      } else {
        recommendation.reasons.push_back("AST scope captures " + std::to_string(scope.target_free_identifiers.size()) + " binding(s)");
        if (!scope.target_function_captures.empty()) recommendation.reasons.push_back("helper function dependencies: " + std::to_string(scope.target_function_captures.size()));
        if (!scope.target_direct_calls.empty()) recommendation.reasons.push_back("direct dependency calls: " + std::to_string(scope.target_direct_calls.size()));
        if (!scope.target_import_captures.empty()) recommendation.reasons.push_back("import dependencies: " + std::to_string(scope.target_import_captures.size()));
        if (!scope.target_constant_captures.empty()) recommendation.reasons.push_back("primitive constant dependencies: " + std::to_string(scope.target_constant_captures.size()));
      }
      recommendation.purity = std::clamp(purity, 0, 100);
      recommendation.runtime = recommendation.purity >= 55 ? "protected" : "manual-review";
      recommendation.confidence = recommendation.purity >= 85 ? 94 : recommendation.purity >= 55 ? 86 : 74;
    }
    recommendation.reasons.push_back("AST-derived function purity score: " + std::to_string(recommendation.purity) + "/100");
    result.push_back(std::move(recommendation));
  }

  std::map<std::string, std::size_t> function_index;
  for (std::size_t i = 0; i < result.size(); ++i) function_index[result[i].name] = i;
  std::vector<int> state(result.size(), 0);
  std::vector<bool> cycle_member(result.size(), false);
  std::function<void(std::size_t, std::vector<std::size_t>&)> resolve = [&](std::size_t index, std::vector<std::size_t>& stack) {
    if (state[index] == 2) return;
    if (state[index] == 1) {
      const auto found = std::find(stack.begin(), stack.end(), index);
      if (found != stack.end()) for (auto it = found; it != stack.end(); ++it) cycle_member[*it] = true;
      return;
    }
    state[index] = 1;
    stack.push_back(index);
    for (const auto& dependency_name : dependency_calls[index]) {
      const auto dependency = function_index.find(dependency_name);
      if (dependency == function_index.end()) continue;
      resolve(dependency->second, stack);
      if (result[index].source == "annotation") continue;
      const auto& dependency_result = result[dependency->second];
      if (dependency_result.runtime == "browser") {
        result[index].runtime = "browser";
        result[index].confidence = std::max(result[index].confidence, 98);
        result[index].purity = std::min(result[index].purity, 15);
        result[index].reasons.push_back("calls browser-bound dependency: " + dependency_name);
      } else if (dependency_result.runtime == "manual-review" && result[index].runtime != "browser") {
        result[index].runtime = "manual-review";
        result[index].confidence = std::max(result[index].confidence, 96);
        result[index].purity = std::min(result[index].purity, 30);
        result[index].reasons.push_back("dependency closure requires review: " + dependency_name);
      }
    }
    stack.pop_back();
    state[index] = 2;
  };
  for (std::size_t i = 0; i < result.size(); ++i) {
    std::vector<std::size_t> stack;
    resolve(i, stack);
  }
  for (std::size_t i = 0; i < result.size(); ++i) {
    if (cycle_member[i]) result[i].reasons.push_back("dependency cycle validated within local function closure");
  }
  return result;
}

PlanResult make_plan(PlannerOptions options){
  load_config_rules(options.input,options);PlanResult result;
  if(!std::filesystem::exists(options.input))raise_error("VENOM-E5000", "planner input does not exist: "+options.input.string());
  for(auto iterator=std::filesystem::recursive_directory_iterator(options.input);
      iterator!=std::filesystem::recursive_directory_iterator();++iterator){
    const auto& entry=*iterator;
    if(entry.is_symlink())raise_error("VENOM-E1100", "input symlinks are not allowed: "+entry.path().string());
    if(entry.is_directory()){
      const auto relative=std::filesystem::relative(entry.path(),options.input).generic_string();
      if(relative==".git"||relative==".venom"||relative==".venom-cache"||
         path_is_within(entry.path(),options.excluded_output))iterator.disable_recursion_pending();
      continue;
    }
    if(!entry.is_regular_file()||!is_js(entry.path()))continue;
    const auto relative=std::filesystem::relative(entry.path(),options.input);std::ifstream in(entry.path(),std::ios::binary);std::ostringstream buffer;buffer<<in.rdbuf();const auto source=buffer.str();
    FileRecommendation file;file.file=relative;file.functions=analyze_functions(source, relative.generic_string());file.runtime="protected";file.confidence=86;file.reasons={"protected-by-default"};
    try {
      auto module_source = source;
      const auto ext = lower(relative.extension().string());
      if (ext == ".ts" || ext == ".tsx") module_source = typescript::transpile(source, relative.generic_string()).javascript;
      const auto parsed = frontend::parse(module_source, relative.generic_string(), true);
      for (const auto& ref : parsed.module_references) file.module_dependencies.emplace_back(ref.specifier);
      for (const auto& imported : parsed.module_imports) {
        if (imported.side_effect_only) file.side_effect_imports.insert(imported.specifier);
        for (const auto& binding : imported.bindings) {
          file.imported_bindings[binding.local_name] = {
            imported.specifier, binding.imported_name, binding.namespace_import
          };
        }
      }
      for (const auto& unsupported : parsed.unsupported_exports)
        file.unresolved_export_kinds.push_back(unsupported.kind);
      file.module_features = parsed.module_features;
      for (const auto& binding : parsed.export_bindings) {
        if (!binding.specifier.empty()) {
          if (binding.export_all) file.export_all_specifiers.push_back(binding.specifier);
          else file.explicit_reexports[binding.exported_name] = {
            binding.specifier, binding.local_name, binding.namespace_export
          };
          continue;
        }
        if (!binding.exported_name.empty() && !binding.local_name.empty())
          file.local_export_bindings[binding.exported_name] = binding.local_name;
      }
    } catch (...) {
      // Script-mode inputs may not form an ES module; function planning already handles that fallback.
    }
    const auto lower_source=lower(source);if(lower_source.find("@venom:")!=std::string::npos){const auto first_code=lower_source.find_first_not_of(" \t\r\n");const auto browser=lower_source.find("@venom: browser");const auto protect=lower_source.find("@venom: protected");if(browser!=std::string::npos&&browser<1024&&(first_code==std::string::npos||browser<first_code+1024)){file.runtime="browser";file.confidence=100;file.source="annotation";file.reasons={"explicit file-level browser annotation"};}else if(protect!=std::string::npos&&protect<1024){file.runtime="protected";file.confidence=100;file.source="annotation";file.reasons={"explicit file-level protected annotation"};}}
    if(file.source=="annotation") {
      for(auto& fn:file.functions) {
        fn.runtime=file.runtime; fn.confidence=100; fn.source="annotation";
        fn.reasons={"inherited explicit file-level annotation"};
      }
    }
    if(file.source!="annotation"&&!file.functions.empty()){
      const auto browser_count=std::count_if(file.functions.begin(),file.functions.end(),[](const auto& f){return f.runtime=="browser";});const auto review_count=std::count_if(file.functions.begin(),file.functions.end(),[](const auto& f){return f.runtime=="manual-review";});
      if(review_count>0){file.runtime="manual-review";file.confidence=100;file.reasons={"contains function requiring manual review"};}
      else if(browser_count>0){file.runtime="browser";file.confidence=94;file.reasons={"contains browser-bound function; conservative whole-file recommendation"};}
      else {file.runtime="protected";file.confidence=88;file.reasons={"all discovered functions are compatible with protected execution"};}
    }
    if (file.source != "annotation" && !file.unresolved_export_kinds.empty()) {
      file.runtime = "manual-review";
      file.confidence = 100;
      for (const auto& kind : file.unresolved_export_kinds)
        file.reasons.push_back("export form requires identity review: " + kind);
    }
    if (file.source != "annotation" && !file.module_features.empty()) {
      file.runtime = "manual-review";
      file.confidence = 100;
      for (const auto& feature : file.module_features)
        file.reasons.push_back("module execution boundary requires review: " + feature);
      for (auto& fn : file.functions) {
        if (fn.source == "annotation") continue;
        fn.runtime = "manual-review";
        fn.confidence = 100;
        fn.purity = std::min(fn.purity, 20);
        fn.reasons.push_back("module execution boundary requires review: " + file.module_features.front());
      }
    }
    auto apply=[&](const std::vector<std::string>& patterns,const std::string& runtime,const std::string& source_name){for(const auto& p:patterns)if(pattern_match(relative,p)){file.runtime=runtime;file.confidence=100;file.source=source_name;file.reasons={"explicit "+source_name+" override: "+p};for(auto& fn:file.functions){fn.runtime=runtime;fn.confidence=100;fn.source=source_name;fn.reasons={"inherited file override: "+p};}}};
    apply(options.config_browser_patterns,"browser","config-rule");apply(options.config_protected_patterns,"protected","config-rule");apply(options.browser_patterns,"browser","manual-cli");apply(options.protected_patterns,"protected","manual-cli");
    for (const auto& export_binding : file.local_export_bindings) {
      const auto& exported_name = export_binding.first;
      const auto& local_name = export_binding.second;
      const auto function = std::find_if(file.functions.begin(), file.functions.end(), [&local_name](const auto& fn) {
        return fn.name == local_name;
      });
      file.exported_symbol_runtimes[exported_name] =
          function != file.functions.end() ? function->runtime : "manual-review";
    }
    result.files[relative]=std::move(file);
  }
  propagate_module_runtimes(result);
  for(const auto& [_,file]:result.files){if(file.runtime=="protected")++result.protected_files;else if(file.runtime=="browser")++result.browser_files;else ++result.review_files;for(const auto& fn:file.functions){if(fn.runtime=="protected")++result.protected_functions;else if(fn.runtime=="browser")++result.browser_functions;else ++result.review_functions;if(fn.runtime=="manual-review"||(fn.confidence<options.minimum_confidence&&fn.source=="planner"))result.strict_pass=false;}if(file.runtime=="manual-review"||(file.confidence<options.minimum_confidence&&file.source=="planner"))result.strict_pass=false;}
  return result;
}
void print_plan(const PlanResult& plan,const PlannerOptions& options){
  if(options.format==OutputFormat::Json){std::cout<<"{\"schema_version\":2,\"planner\":\"venom-local-protection-planner\",\"mode\":\""<<esc(options.mode)<<"\",\"minimum_confidence\":"<<options.minimum_confidence<<",\"precedence\":[\"annotations\",\"cli-overrides\",\"config-rules\",\"planner\",\"protected-default\"],\"server_required\":false,\"summary\":{\"protected_files\":"<<plan.protected_files<<",\"browser_files\":"<<plan.browser_files<<",\"review_files\":"<<plan.review_files<<",\"protected_functions\":"<<plan.protected_functions<<",\"browser_functions\":"<<plan.browser_functions<<",\"review_functions\":"<<plan.review_functions<<"},\"recommendations\":[";bool first_file=true;for(const auto& [_,f]:plan.files){if(!first_file)std::cout<<',';first_file=false;std::cout<<"{\"file\":\""<<esc(f.file.generic_string())<<"\",\"runtime\":\""<<f.runtime<<"\",\"confidence\":"<<f.confidence<<",\"source\":\""<<f.source<<"\",\"reasons\":[";for(size_t i=0;i<f.reasons.size();++i){if(i)std::cout<<',';std::cout<<'"'<<esc(f.reasons[i])<<'"';}std::cout<<"],\"functions\":[";for(size_t i=0;i<f.functions.size();++i){if(i)std::cout<<',';const auto& fn=f.functions[i];std::cout<<"{\"name\":\""<<esc(fn.name)<<"\",\"line\":"<<fn.line<<",\"runtime\":\""<<fn.runtime<<"\",\"confidence\":"<<fn.confidence<<",\"purity\":"<<fn.purity<<",\"source\":\""<<fn.source<<"\",\"reasons\":[";for(size_t j=0;j<fn.reasons.size();++j){if(j)std::cout<<',';std::cout<<'"'<<esc(fn.reasons[j])<<'"';}std::cout<<"]}";}std::cout<<"]}";}std::cout<<"],\"strict_pass\":"<<(plan.strict_pass?"true":"false")<<"}\n";return;}
  std::cout<<"Venom protection plan\n\nMode: "<<options.mode<<"\nServer required: no\nMinimum confidence: "<<options.minimum_confidence<<"%\nPrecedence: annotations > CLI overrides > config rules > planner > protected default\n\n";
  for(const auto& [_,f]:plan.files){std::cout<<'['<<(f.runtime=="protected"?"PROTECT":f.runtime=="browser"?"BROWSER":"REVIEW")<<"] "<<f.file.string()<<"  confidence="<<f.confidence<<"%  source="<<f.source<<"\n";for(const auto& reason:f.reasons)std::cout<<"          - "<<reason<<"\n";for(const auto& fn:f.functions){std::cout<<"          "<<(fn.runtime=="protected"?"+":fn.runtime=="browser"?"@":"!")<<" "<<fn.name<<" (line "<<fn.line<<") -> "<<fn.runtime<<", confidence="<<fn.confidence<<"%, purity="<<fn.purity<<"\n";}}
  std::cout<<"\nSummary: "<<plan.protected_files<<" protected, "<<plan.browser_files<<" browser, "<<plan.review_files<<" review files; "<<plan.protected_functions<<" protected, "<<plan.browser_functions<<" browser, "<<plan.review_functions<<" review functions.\n";
  std::cout<<"The planner is advisory. Existing @venom annotations and manual flags remain authoritative.\n";if(options.mode=="strict")std::cout<<"Strict result: "<<(plan.strict_pass?"PASS":"FAIL")<<"\n";
}
}

bool run_protection_plan(const PlannerOptions& requested){PlannerOptions options=requested;const auto plan=make_plan(options);print_plan(plan,options);if(!options.report_file.empty()){std::ofstream out(options.report_file);if(!out)raise_error("VENOM-E5000", "failed to write planner report: "+options.report_file.string());auto old=std::cout.rdbuf(out.rdbuf());PlannerOptions json=options;json.format=OutputFormat::Json;print_plan(plan,json);std::cout.rdbuf(old);}return options.mode!="strict"||plan.strict_pass;}
bool enforce_build_protection_plan(const BuildOptions& options){if(options.planner_mode=="off"||options.planner_mode=="manual")return true;PlannerOptions plan;plan.input=options.input;plan.excluded_output=options.output;plan.mode=options.planner_mode;plan.protected_patterns=options.force_protected;plan.browser_patterns=options.force_browser;plan.minimum_confidence=options.planner_minimum_confidence;const auto result=make_plan(plan);std::cout<<"[venom] planner: "<<result.protected_files<<" protected, "<<result.browser_files<<" browser, "<<result.review_files<<" review files\n";if(options.planner_mode=="strict"&&!result.strict_pass)raise_error("VENOM-E5000", "strict planner rejected unresolved or low-confidence recommendations; run 'venom plan --mode strict' for details or add manual overrides");return true;}
}

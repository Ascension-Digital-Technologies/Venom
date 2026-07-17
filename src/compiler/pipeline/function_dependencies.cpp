#include "compiler/pipeline/function_dependencies.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace venom::compiler {
namespace {

struct Declaration {
  enum class Kind { Function, PrimitiveConstant, MutableBinding, UnsupportedConstant, ImportBinding };
  Kind kind = Kind::Function;
  std::string name;
  std::string declaration;
  std::string params;
};

std::string lower_ascii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

std::string trim_copy(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return {};
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1u);
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

int brace_depth_at(const std::string& masked, std::size_t offset) {
  int depth = 0;
  for (std::size_t i=0;i<std::min(offset,masked.size());++i) {
    if (masked[i]=='{') ++depth;
    else if (masked[i]=='}') --depth;
  }
  return depth;
}

bool extract_function_at(const std::string& source, std::size_t begin,
                         std::string& declaration, std::string& params, std::string& name) {
  const auto line_end=source.find('\n',begin);
  const auto line=source.substr(begin,(line_end==std::string::npos?source.size():line_end)-begin);
  const std::regex header(R"(^\s*(?:export\s+)?(?:default\s+)?(?:async\s+)?function\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*\(([^)]*)\)\s*\{)");
  std::smatch match;
  if(!std::regex_search(line,match,header)||match.size()<3) return false;
  name=match[1].str(); params=match[2].str();
  const auto brace=source.find('{',begin+static_cast<std::size_t>(match.position(0)));
  if(brace==std::string::npos) return false;
  int depth=0; bool single=false,dbl=false,templ=false,line_comment=false,block_comment=false,escape=false;
  for(std::size_t i=brace;i<source.size();++i){
    const char c=source[i]; const char n=i+1u<source.size()?source[i+1u]:'\0';
    if(line_comment){if(c=='\n')line_comment=false;continue;}
    if(block_comment){if(c=='*'&&n=='/'){block_comment=false;++i;}continue;}
    if(escape){escape=false;continue;}
    if((single||dbl||templ)&&c=='\\'){escape=true;continue;}
    if(!single&&!dbl&&!templ&&c=='/'&&n=='/'){line_comment=true;++i;continue;}
    if(!single&&!dbl&&!templ&&c=='/'&&n=='*'){block_comment=true;++i;continue;}
    if(!dbl&&!templ&&c=='\''){single=!single;continue;}
    if(!single&&!templ&&c=='"'){dbl=!dbl;continue;}
    if(!single&&!dbl&&c=='`'){templ=!templ;continue;}
    if(single||dbl||templ)continue;
    if(c=='{')++depth;
    else if(c=='}'&&--depth==0){declaration=trim_copy(source.substr(begin,i+1u-begin));return true;}
  }
  return false;
}

std::unordered_map<std::string,Declaration> index_declarations(const std::string& source) {
  std::unordered_map<std::string,Declaration> out;
  const auto masked=mask_literals_and_comments(source);
  const std::regex function_line(R"(^\s*(?:export\s+)?(?:default\s+)?(?:async\s+)?function\s+)");
  const std::regex primitive(R"(^\s*const\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*=\s*((?:true|false|null)|(?:[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?)|(?:"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'))\s*;)");
  const std::regex mutable_binding(R"(^\s*(?:let|var)\s+([A-Za-z_$][A-Za-z0-9_$]*)\b)");
  const std::regex const_binding(R"(^\s*const\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*=)");
  const std::regex import_binding(R"(^\s*import\s+(?:\{[^}]*\}|\*\s+as\s+[A-Za-z_$][A-Za-z0-9_$]*|[A-Za-z_$][A-Za-z0-9_$]*)\s+from\s+)");
  std::size_t offset=0;
  while(offset<=source.size()){
    const auto end=source.find('\n',offset);
    const auto count=(end==std::string::npos?source.size():end)-offset;
    const auto original=source.substr(offset,count);
    const auto masked_line=masked.substr(offset,count);
    if(brace_depth_at(masked,offset)==0){
      if(std::regex_search(masked_line,function_line)){
        std::string declaration,params,name;
        if(extract_function_at(source,offset,declaration,params,name))
          out[name]={Declaration::Kind::Function,name,declaration,params};
      }
      std::smatch match;
      if(std::regex_search(original,match,primitive)&&match.size()>=3){
        const auto name=match[1].str();
        out[name]={Declaration::Kind::PrimitiveConstant,name,"const "+name+"="+match[2].str()+";",{}};
      } else if (std::regex_search(original, match, mutable_binding) && match.size() >= 2) {
        const auto name = match[1].str();
        out[name] = {Declaration::Kind::MutableBinding, name, original, {}};
      } else if (std::regex_search(original, match, const_binding) && match.size() >= 2) {
        const auto name = match[1].str();
        out[name] = {Declaration::Kind::UnsupportedConstant, name, original, {}};
      }
      // Imports are deliberately not lifted yet. Record simple imported names
      // so capture failures are precise rather than reported as unknown globals.
      if (std::regex_search(original, import_binding)) {
        const std::regex imported_name(R"([A-Za-z_$][A-Za-z0-9_$]*)");
        for (auto it = std::sregex_iterator(original.begin(), original.end(), imported_name);
             it != std::sregex_iterator(); ++it) {
          const auto candidate = it->str();
          if (candidate != "import" && candidate != "from" && candidate != "as")
            out.emplace(candidate, Declaration{Declaration::Kind::ImportBinding, candidate, original, {}});
        }
      }
    }
    if(end==std::string::npos)break;
    offset=end+1u;
  }
  return out;
}

std::unordered_set<std::string> identifiers(const std::string& csv) {
  std::unordered_set<std::string> out;
  const std::regex ident(R"([A-Za-z_$][A-Za-z0-9_$]*)");
  for(auto it=std::sregex_iterator(csv.begin(),csv.end(),ident);it!=std::sregex_iterator();++it)out.insert(it->str());
  return out;
}

std::vector<std::string> free_identifiers(const std::string& declaration,
                                          const std::string& function_name,
                                          const std::string& params) {
  auto allowed=identifiers(params); allowed.insert(function_name);
  static const char* builtins[]={"undefined","NaN","Infinity","Math","JSON","Number","String","Boolean","Array","Object","BigInt","Date","RegExp","Map","Set","WeakMap","WeakSet","Promise","Error","TypeError","RangeError","parseInt","parseFloat","isFinite","isNaN","Uint8Array","Uint16Array","Uint32Array","Int8Array","Int16Array","Int32Array","Float32Array","Float64Array","ArrayBuffer","DataView","console"};
  for(const char* n:builtins)allowed.insert(n);
  static const char* keywords[]={"async","function","return","if","else","for","while","do","switch","case","default","break","continue","try","catch","finally","throw","new","typeof","instanceof","in","of","void","delete","true","false","null","const","let","var","class","extends","static","get","set"};
  for(const char* n:keywords)allowed.insert(n);
  const auto masked=mask_literals_and_comments(declaration);
  const std::regex local(R"(\b(?:const|let|var|function|class)\s+([A-Za-z_$][A-Za-z0-9_$]*))");
  for(auto it=std::sregex_iterator(masked.begin(),masked.end(),local);it!=std::sregex_iterator();++it)allowed.insert((*it)[1].str());
  const std::regex caught(R"(\bcatch\s*\(\s*([A-Za-z_$][A-Za-z0-9_$]*))");
  for(auto it=std::sregex_iterator(masked.begin(),masked.end(),caught);it!=std::sregex_iterator();++it)allowed.insert((*it)[1].str());
  const std::regex nested_function_params(R"(\bfunction(?:\s+[A-Za-z_$][A-Za-z0-9_$]*)?\s*\(([^)]*)\))");
  for (auto it = std::sregex_iterator(masked.begin(), masked.end(), nested_function_params);
       it != std::sregex_iterator(); ++it) {
    const auto nested = identifiers((*it)[1].str());
    allowed.insert(nested.begin(), nested.end());
  }
  const std::regex arrow_params(R"(\(([^)]*)\)\s*=>|\b([A-Za-z_$][A-Za-z0-9_$]*)\s*=>)");
  for (auto it = std::sregex_iterator(masked.begin(), masked.end(), arrow_params);
       it != std::sregex_iterator(); ++it) {
    const auto nested = identifiers((*it)[1].matched ? (*it)[1].str() : (*it)[2].str());
    allowed.insert(nested.begin(), nested.end());
  }
  const std::regex ident(R"([A-Za-z_$][A-Za-z0-9_$]*)");
  std::vector<std::string> out; std::unordered_set<std::string> seen;
  for(auto it=std::sregex_iterator(masked.begin(),masked.end(),ident);it!=std::sregex_iterator();++it){
    const auto name=it->str(); const auto at=static_cast<std::size_t>(it->position());
    if (at > 0u && std::isdigit(static_cast<unsigned char>(masked[at - 1u]))) continue;
    std::size_t prev=at;while(prev>0u&&std::isspace(static_cast<unsigned char>(masked[prev-1u])))--prev;
    if(prev>0u&&masked[prev-1u]=='.')continue;
    std::size_t next=at+name.size();while(next<masked.size()&&std::isspace(static_cast<unsigned char>(masked[next])))++next;
    if(next<masked.size()&&masked[next]==':')continue;
    if(!allowed.count(name)&&seen.insert(name).second)out.push_back(name);
  }
  return out;
}

bool realm_bound(const std::string& declaration,std::string& reason){
  static const std::pair<std::string_view,std::string_view> unsafe[]={
    {"document","DOM access"},{"window","browser global access"},
    {"eval(","dynamic eval"},{"super","super binding"},{"new.target","new.target binding"},
    {"yield","generator semantics"},{"import.meta","module metadata"}};
  const auto lower=lower_ascii(mask_literals_and_comments(declaration));
  for(const auto& item:unsafe)if(lower.find(item.first)!=std::string::npos){reason=std::string(item.second);return true;}
  return false;
}

bool collect(const std::string& declaration,const std::string& name,const std::string& params,
             const std::unordered_map<std::string,Declaration>& declarations,
             std::vector<LiftedFunctionDependency>& ordered,
             std::unordered_set<std::string>& resolved,
             std::unordered_set<std::string>& visiting,
             std::string& reason){
  for(const auto& capture:free_identifiers(declaration,name,params)){
    if(resolved.count(capture))continue;
    static const std::unordered_set<std::string> browser_globals = {
      "window", "document", "navigator", "location", "history", "localStorage",
      "sessionStorage", "fetch", "XMLHttpRequest", "WebSocket", "Worker",
      "HTMLElement", "customElements", "requestAnimationFrame"
    };
    if (browser_globals.count(capture)) {
      reason = "browser-only lexical capture requires an explicit capability: " + capture;
      return false;
    }
    const auto found=declarations.find(capture);
    if(found==declarations.end()){reason="unresolved lexical capture: "+capture;return false;}
    if(visiting.count(capture))continue;
    visiting.insert(capture);
    const auto& dep=found->second;
    if (dep.kind == Declaration::Kind::MutableBinding) {
      reason = "mutable lexical capture is not safe to lift: " + capture;
      return false;
    }
    if (dep.kind == Declaration::Kind::UnsupportedConstant) {
      reason = "constant capture is not a supported immutable literal: " + capture;
      return false;
    }
    if (dep.kind == Declaration::Kind::ImportBinding) {
      reason = "imported lexical capture requires module dependency lowering: " + capture;
      return false;
    }
    if(dep.kind==Declaration::Kind::Function){
      std::string blocker;
      if(realm_bound(dep.declaration,blocker)){reason="helper "+capture+" is realm-bound: "+blocker;return false;}
      if(!collect(dep.declaration,dep.name,dep.params,declarations,ordered,resolved,visiting,reason))return false;
    }
    visiting.erase(capture);
    if(resolved.insert(capture).second)ordered.push_back({dep.name,dep.declaration});
  }
  return true;
}

} // namespace

FunctionDependencyResolution resolve_liftable_function_dependencies(
    const std::string& source,const std::string& target_declaration,
    const std::string& target_name,const std::string& target_params){
  FunctionDependencyResolution result;
  const auto declarations=index_declarations(source);
  std::unordered_set<std::string> resolved;
  std::unordered_set<std::string> visiting{target_name};
  result.success=collect(target_declaration,target_name,target_params,declarations,
                         result.dependencies,resolved,visiting,result.reason);
  return result;
}

bool verify_isolated_protected_unit(const std::string& declaration, std::string& reason) {
  if (declaration.size() < 16384u) {
    reason = "isolated fallback is reserved for large self-contained compilation units";
    return false;
  }
  std::string blocker;
  if (realm_bound(declaration, blocker)) {
    reason = "isolated unit is realm-bound: " + blocker;
    return false;
  }
  const auto masked = lower_ascii(mask_literals_and_comments(declaration));
  static const std::pair<std::string_view, std::string_view> browser_access[] = {
    {"document.", "document"}, {"window.", "window"},
    {"globalthis.document", "document"}, {"globalthis.window", "window"},
    {"navigator.", "navigator"}, {"localstorage.", "localStorage"},
    {"sessionstorage.", "sessionStorage"}
  };
  for (const auto& item : browser_access) {
    if (masked.find(item.first) != std::string::npos) {
      reason = "isolated unit captures browser-only global: " + std::string(item.second);
      return false;
    }
  }
  reason = "verified isolated protected compilation unit";
  return true;
}

} // namespace venom::compiler

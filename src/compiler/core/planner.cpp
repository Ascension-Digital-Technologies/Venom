#include "compiler/core/planner.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace venom::compiler {
namespace {
struct FunctionRecommendation {
  std::string name;
  std::uint32_t line = 0;
  std::string realm = "protected";
  int confidence = 70;
  int purity = 50;
  std::string source = "planner";
  std::vector<std::string> reasons;
};
struct FileRecommendation {
  std::filesystem::path file;
  std::string realm = "protected";
  int confidence = 70;
  std::string source = "planner";
  std::vector<std::string> reasons;
  std::vector<FunctionRecommendation> functions;
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
  for(unsigned char c:s){
    if(c=='"')o<<"\\\""; else if(c=='\\')o<<"\\\\"; else if(c=='\n')o<<"\\n";
    else if(c=='\r')o<<"\\r"; else if(c=='\t')o<<"\\t"; else if(c<0x20)o<<'?'; else o<<static_cast<char>(c);
  }
  return o.str();
}
bool is_js(const std::filesystem::path& p){
  const auto e=lower(p.extension().string());
  return e==".js"||e==".mjs"||e==".cjs"||e==".jsx"||e==".ts"||e==".tsx";
}
bool contains_any(const std::string& body,const std::vector<std::string>& markers,std::string* matched=nullptr){
  const auto l=lower(body);
  for(const auto& marker:markers) if(l.find(lower(marker))!=std::string::npos){if(matched)*matched=marker;return true;}
  return false;
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

std::string extract_function_body(const std::vector<std::string>& lines,std::size_t start){
  std::ostringstream out; int depth=0; bool opened=false; bool single=false,dbl=false,tpl=false,escape=false;
  for(std::size_t i=start;i<lines.size();++i){
    out<<lines[i]<<'\n';
    for(char c:lines[i]){
      if(escape){escape=false;continue;} if((single||dbl||tpl)&&c=='\\'){escape=true;continue;}
      if(!dbl&&!tpl&&c=='\''){single=!single;continue;} if(!single&&!tpl&&c=='"'){dbl=!dbl;continue;} if(!single&&!dbl&&c=='`'){tpl=!tpl;continue;}
      if(single||dbl||tpl)continue; if(c=='{'){++depth;opened=true;}else if(c=='}')--depth;
    }
    if(opened&&depth<=0)break; if(!opened&&lines[i].find("=>")!=std::string::npos&&lines[i].find(';')!=std::string::npos)break;
  }
  return out.str();
}
std::string pending_annotation(const std::vector<std::string>& lines,std::size_t start){
  for(std::size_t back=1;back<=3&&back<=start;++back){const auto text=lower(lines[start-back]);if(text.find("@venom:")!=std::string::npos){if(text.find("browser")!=std::string::npos)return "browser";if(text.find("protected")!=std::string::npos)return "protected";}if(!trim(lines[start-back]).empty()&&text.find("//")!=0&&text.find("/*")==std::string::npos)break;}
  return {};
}
std::vector<FunctionRecommendation> analyze_functions(const std::string& source){
  std::vector<std::string> lines;
  std::istringstream input(source);
  std::string line;
  while(std::getline(input,line)) lines.push_back(line);
  const std::regex decl(R"(^\s*(?:(?:export\s+)?(?:default\s+)?)?(?:async\s+)?function\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*\(|^\s*(?:export\s+)?(?:const|let|var)\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*=\s*(?:async\s*)?(?:\([^)]*\)|[A-Za-z_$][A-Za-z0-9_$]*)\s*=>)");
  static const std::vector<std::string> browser={"document.","window.","navigator.","localStorage","sessionStorage","querySelector","getElementById","createElement","addEventListener","requestAnimationFrame","CanvasRenderingContext2D","WebGL","fetch(","XMLHttpRequest","WebSocket("};
  static const std::vector<std::string> dynamic={"eval(","new Function(","WebAssembly.compile(","import("};
  static const std::vector<std::string> side_effect={"console.","Date.now(","Math.random(","crypto.getRandomValues","setTimeout(","setInterval(","postMessage("};
  static const std::vector<std::string> pure={"Math.","reduce(","map(","filter(","sort(","JSON.stringify","JSON.parse"};
  std::vector<FunctionRecommendation> result;
  for(std::size_t i=0;i<lines.size();++i){
    std::smatch m;
    if(!std::regex_search(lines[i],m,decl)) continue;
    std::string name=m[1].matched?m[1].str():m[2].str();
    if(name.empty()) continue;
    FunctionRecommendation r;
    r.name=name;
    r.line=static_cast<std::uint32_t>(i+1);
    const auto body=extract_function_body(lines,i);
    const auto annotation=pending_annotation(lines,i);
    std::string marker;
    if(!annotation.empty()){
      r.realm=annotation; r.confidence=100; r.purity=annotation=="protected"?90:20; r.source="annotation";
      r.reasons={"explicit @venom function annotation"};
    } else if(contains_any(body,dynamic,&marker)){
      r.realm="manual-review"; r.confidence=100; r.purity=10;
      r.reasons={"dynamic source or runtime loading: "+marker};
    } else if(contains_any(body,browser,&marker)){
      r.realm="browser"; r.confidence=96; r.purity=15;
      r.reasons={"direct browser capability: "+marker};
    } else {
      int purity=65;
      if(contains_any(body,pure,&marker)){purity+=15;r.reasons.push_back("deterministic computation signal: "+marker);}
      if(contains_any(body,side_effect,&marker)){purity-=25;r.reasons.push_back("observable side effect: "+marker);}
      purity=std::clamp(purity,0,100);
      r.purity=purity; r.realm=purity>=55?"protected":"manual-review";
      r.confidence=purity>=80?92:purity>=55?82:72;
      if(r.reasons.empty()) r.reasons.push_back("no browser-only capability detected");
    }
    r.reasons.push_back("function purity score: "+std::to_string(r.purity)+"/100");
    result.push_back(std::move(r));
  }
  return result;
}

PlanResult make_plan(PlannerOptions options){
  load_config_rules(options.input,options);PlanResult result;
  if(!std::filesystem::exists(options.input))throw std::runtime_error("planner input does not exist: "+options.input.string());
  for(const auto& entry:std::filesystem::recursive_directory_iterator(options.input)){
    if(!entry.is_regular_file()||!is_js(entry.path()))continue;
    const auto relative=std::filesystem::relative(entry.path(),options.input);std::ifstream in(entry.path(),std::ios::binary);std::ostringstream buffer;buffer<<in.rdbuf();const auto source=buffer.str();
    FileRecommendation file;file.file=relative;file.functions=analyze_functions(source);file.realm="protected";file.confidence=86;file.reasons={"protected-by-default"};
    const auto lower_source=lower(source);if(lower_source.find("@venom:")!=std::string::npos){const auto first_code=lower_source.find_first_not_of(" \t\r\n");const auto browser=lower_source.find("@venom: browser");const auto protect=lower_source.find("@venom: protected");if(browser!=std::string::npos&&browser<1024&&(first_code==std::string::npos||browser<first_code+1024)){file.realm="browser";file.confidence=100;file.source="annotation";file.reasons={"explicit file-level browser annotation"};}else if(protect!=std::string::npos&&protect<1024){file.realm="protected";file.confidence=100;file.source="annotation";file.reasons={"explicit file-level protected annotation"};}}
    if(file.source=="annotation") {
      for(auto& fn:file.functions) {
        fn.realm=file.realm; fn.confidence=100; fn.source="annotation";
        fn.reasons={"inherited explicit file-level annotation"};
      }
    }
    if(file.source!="annotation"&&!file.functions.empty()){
      const auto browser_count=std::count_if(file.functions.begin(),file.functions.end(),[](const auto& f){return f.realm=="browser";});const auto review_count=std::count_if(file.functions.begin(),file.functions.end(),[](const auto& f){return f.realm=="manual-review";});
      if(review_count>0){file.realm="manual-review";file.confidence=100;file.reasons={"contains function requiring manual review"};}
      else if(browser_count>0){file.realm="browser";file.confidence=94;file.reasons={"contains browser-bound function; conservative whole-file recommendation"};}
      else {file.realm="protected";file.confidence=88;file.reasons={"all discovered functions are compatible with protected execution"};}
    }
    auto apply=[&](const std::vector<std::string>& patterns,const std::string& realm,const std::string& source_name){for(const auto& p:patterns)if(pattern_match(relative,p)){file.realm=realm;file.confidence=100;file.source=source_name;file.reasons={"explicit "+source_name+" override: "+p};for(auto& fn:file.functions){fn.realm=realm;fn.confidence=100;fn.source=source_name;fn.reasons={"inherited file override: "+p};}}};
    apply(options.config_browser_patterns,"browser","config-rule");apply(options.config_protected_patterns,"protected","config-rule");apply(options.browser_patterns,"browser","manual-cli");apply(options.protected_patterns,"protected","manual-cli");
    result.files[relative]=std::move(file);
  }
  for(const auto& [_,file]:result.files){if(file.realm=="protected")++result.protected_files;else if(file.realm=="browser")++result.browser_files;else ++result.review_files;for(const auto& fn:file.functions){if(fn.realm=="protected")++result.protected_functions;else if(fn.realm=="browser")++result.browser_functions;else ++result.review_functions;if(fn.realm=="manual-review"||(fn.confidence<options.minimum_confidence&&fn.source=="planner"))result.strict_pass=false;}if(file.realm=="manual-review"||(file.confidence<options.minimum_confidence&&file.source=="planner"))result.strict_pass=false;}
  return result;
}
void print_plan(const PlanResult& plan,const PlannerOptions& options){
  if(options.format==OutputFormat::Json){std::cout<<"{\"schema_version\":2,\"planner\":\"venom-local-protection-planner\",\"mode\":\""<<esc(options.mode)<<"\",\"minimum_confidence\":"<<options.minimum_confidence<<",\"precedence\":[\"annotations\",\"cli-overrides\",\"config-rules\",\"planner\",\"protected-default\"],\"server_required\":false,\"summary\":{\"protected_files\":"<<plan.protected_files<<",\"browser_files\":"<<plan.browser_files<<",\"review_files\":"<<plan.review_files<<",\"protected_functions\":"<<plan.protected_functions<<",\"browser_functions\":"<<plan.browser_functions<<",\"review_functions\":"<<plan.review_functions<<"},\"recommendations\":[";bool first_file=true;for(const auto& [_,f]:plan.files){if(!first_file)std::cout<<',';first_file=false;std::cout<<"{\"file\":\""<<esc(f.file.generic_string())<<"\",\"realm\":\""<<f.realm<<"\",\"confidence\":"<<f.confidence<<",\"source\":\""<<f.source<<"\",\"reasons\":[";for(size_t i=0;i<f.reasons.size();++i){if(i)std::cout<<',';std::cout<<'"'<<esc(f.reasons[i])<<'"';}std::cout<<"],\"functions\":[";for(size_t i=0;i<f.functions.size();++i){if(i)std::cout<<',';const auto& fn=f.functions[i];std::cout<<"{\"name\":\""<<esc(fn.name)<<"\",\"line\":"<<fn.line<<",\"realm\":\""<<fn.realm<<"\",\"confidence\":"<<fn.confidence<<",\"purity\":"<<fn.purity<<",\"source\":\""<<fn.source<<"\",\"reasons\":[";for(size_t j=0;j<fn.reasons.size();++j){if(j)std::cout<<',';std::cout<<'"'<<esc(fn.reasons[j])<<'"';}std::cout<<"]}";}std::cout<<"]}";}std::cout<<"],\"strict_pass\":"<<(plan.strict_pass?"true":"false")<<"}\n";return;}
  std::cout<<"Venom protection plan\n\nMode: "<<options.mode<<"\nServer required: no\nMinimum confidence: "<<options.minimum_confidence<<"%\nPrecedence: annotations > CLI overrides > config rules > planner > protected default\n\n";
  for(const auto& [_,f]:plan.files){std::cout<<'['<<(f.realm=="protected"?"PROTECT":f.realm=="browser"?"BROWSER":"REVIEW")<<"] "<<f.file.string()<<"  confidence="<<f.confidence<<"%  source="<<f.source<<"\n";for(const auto& reason:f.reasons)std::cout<<"          - "<<reason<<"\n";for(const auto& fn:f.functions){std::cout<<"          "<<(fn.realm=="protected"?"+":fn.realm=="browser"?"@":"!")<<" "<<fn.name<<" (line "<<fn.line<<") -> "<<fn.realm<<", confidence="<<fn.confidence<<"%, purity="<<fn.purity<<"\n";}}
  std::cout<<"\nSummary: "<<plan.protected_files<<" protected, "<<plan.browser_files<<" browser, "<<plan.review_files<<" review files; "<<plan.protected_functions<<" protected, "<<plan.browser_functions<<" browser, "<<plan.review_functions<<" review functions.\n";
  std::cout<<"The planner is advisory. Existing @venom annotations and manual flags remain authoritative.\n";if(options.mode=="strict")std::cout<<"Strict result: "<<(plan.strict_pass?"PASS":"FAIL")<<"\n";
}
}

bool run_protection_plan(const PlannerOptions& requested){PlannerOptions options=requested;const auto plan=make_plan(options);print_plan(plan,options);if(!options.report_file.empty()){std::ofstream out(options.report_file);if(!out)throw std::runtime_error("failed to write planner report: "+options.report_file.string());auto old=std::cout.rdbuf(out.rdbuf());PlannerOptions json=options;json.format=OutputFormat::Json;print_plan(plan,json);std::cout.rdbuf(old);}return options.mode!="strict"||plan.strict_pass;}
bool enforce_build_protection_plan(const BuildOptions& options){if(options.planner_mode=="off"||options.planner_mode=="manual")return true;PlannerOptions plan;plan.input=options.input;plan.mode=options.planner_mode;plan.protected_patterns=options.force_protected;plan.browser_patterns=options.force_browser;plan.minimum_confidence=options.planner_minimum_confidence;const auto result=make_plan(plan);std::cout<<"[venom] planner: "<<result.protected_files<<" protected, "<<result.browser_files<<" browser, "<<result.review_files<<" review files\n";if(options.planner_mode=="strict"&&!result.strict_pass)throw std::runtime_error("strict planner rejected unresolved or low-confidence recommendations; run 'venom plan --mode strict' for details or add manual overrides");return true;}
}
